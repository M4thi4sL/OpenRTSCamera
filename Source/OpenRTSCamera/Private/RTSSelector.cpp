// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSSelector.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "RTSHUD.h"
#include "Interfaces/RTSSelection.h"

// Sets default values for this component's properties
URTSSelector::URTSSelector(): PlayerController(nullptr), HUD(nullptr), bIsSelecting(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Add defaults for input actions
	static ConstructorHelpers::FObjectFinder<UInputAction>
		BeginSelectionActionFinder(TEXT("/OpenRTSCamera/Inputs/BeginSelection"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>
		InputMappingContextFinder(TEXT("/OpenRTSCamera/Inputs/OpenRTSCameraInputs"));
	BeginSelection = BeginSelectionActionFinder.Object;
	InputMappingContext = InputMappingContextFinder.Object;
}


// Called when the game starts
void URTSSelector::BeginPlay()
{
	Super::BeginPlay();

	const auto NetMode = GetNetMode();
	if (NetMode != NM_DedicatedServer)
	{
		CollectComponentDependencyReferences();
		BindInputMappingContext();
		BindInputActions();
		OnActorsSelected.AddDynamic(this, &URTSSelector::HandleSelectedActors);
	}
}

void URTSSelector::HandleSelectedActors_Implementation(const TArray<AActor*>& NewSelectedActors)
{
	// Convert NewSelectedActors to a set for efficient lookup
	TSet<AActor*> FilteredSelectedActors;
	
	for (AActor* Actor : NewSelectedActors)
	{
		if (Actor && Actor->Implements<URTSSelection>())
		{
			FilteredSelectedActors.Add(Actor);
		}
	}

	// Iterate over currently selected actors and deselect those that are no longer selected.
	for (AActor* Selected : SelectedActors)
	{
		if (Selected && !FilteredSelectedActors.Contains(Selected))
		{
			if (Selected && Selected->Implements<URTSSelection>())
			{
				IRTSSelection::Execute_OnDeselected(Selected);
	
			}
		}
	}

	// Clear the current selection AFTER deselecting actors
	SelectedActors.Empty();	
	
	// Add new selected actors and call OnSelected
	for (AActor* Actor : FilteredSelectedActors)
	{
		SelectedActors.Add(Actor);
		IRTSSelection::Execute_OnSelected(Actor);
	}
}

void URTSSelector::ClearSelectedActors_Implementation()
{
	SelectedActors.Empty();
}

void URTSSelector::CollectComponentDependencyReferences()
{
	if (const auto PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PlayerController = PlayerControllerRef;
		HUD = Cast<ARTSHUD>(PlayerControllerRef->GetHUD());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USelector is not attached to a PlayerController."));
	}
}

void URTSSelector::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (const auto InputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		InputComponent->BindAction(BeginSelection, ETriggerEvent::Started, this, &URTSSelector::OnSelectionStart);
		InputComponent->BindAction(BeginSelection, ETriggerEvent::Completed, this, &URTSSelector::OnSelectionEnd);
	}
}

void URTSSelector::BindInputActions()
{
	if (const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
	{
		EnhancedInputComponent->BindAction(
			BeginSelection,
			ETriggerEvent::Started,
			this,
			&URTSSelector::OnSelectionStart
		);

		EnhancedInputComponent->BindAction(
			BeginSelection,
			ETriggerEvent::Triggered,
			this,
			&URTSSelector::OnUpdateSelection
		);

		EnhancedInputComponent->BindAction(
			BeginSelection,
			ETriggerEvent::Completed,
			this,
			&URTSSelector::OnSelectionEnd
		);
	}
}

void URTSSelector::BindInputMappingContext()
{
	if (PlayerController && PlayerController->GetLocalPlayer())
	{
		if (const auto Input = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			PlayerController->bShowMouseCursor = true;

			// Check if the context is already bound to prevent double binding
			if (!Input->HasMappingContext(InputMappingContext))
			{
				Input->ClearAllMappings();
				Input->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}

void URTSSelector::OnSelectionStart(const FInputActionValue& Value)
{
	FVector2D MousePosition;
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);
	HUD->BeginSelection(MousePosition);
}

void URTSSelector::OnUpdateSelection(const FInputActionValue& Value)
{
	FVector2D MousePosition;
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);
	SelectionEnd = MousePosition;
	HUD->UpdateSelection(SelectionEnd);
}

void URTSSelector::OnSelectionEnd(const FInputActionValue& Value)
{
	// Call PerformSelection on the HUD to execute selection logic
	HUD->EndSelection();
}
