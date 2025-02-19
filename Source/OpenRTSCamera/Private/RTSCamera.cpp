// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSCamera.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "RTSCameraBoundsVolume.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"

URTSCamera::URTSCamera()
{
	PrimaryComponentTick.bCanEverTick = true;
	CollisionChannel = ECC_WorldStatic;
	DragExtent = 0.6f;
	EdgeScrollSpeed = 50;
	DistanceFromEdgeThreshold = 0.1f;
	EnableCameraLag = true;
	EnableCameraRotationLag = true;
	EnableDynamicCameraHeight = true;
	EnableEdgeScrolling = true;
	FindGroundTraceLength = 100000;
	MaximumZoomLength = 5000;
	MinimumZoomLength = 500;
	MoveSpeed = 50;
	RotateAngle = 45;
	StartingYAngle = -45.0f;
	StartingZAngle = 0;
	StartingLenght = 400.0f;
	ZoomCatchupSpeed = 4;
	ZoomSpeed = -200;

	/** this is not really needed but preloads the assets into the defaults section of children */
	static ConstructorHelpers::FObjectFinder<UInputAction>
		MoveCameraXAxisFinder(TEXT("/OpenRTSCamera/Inputs/MoveCameraXAxis"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		MoveCameraYAxisFinder(TEXT("/OpenRTSCamera/Inputs/MoveCameraYAxis"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		TurnCameraLeftFinder(TEXT("/OpenRTSCamera/Inputs/TurnCameraLeft"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		TurnCameraRightFinder(TEXT("/OpenRTSCamera/Inputs/TurnCameraRight"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		ZoomCameraFinder(TEXT("/OpenRTSCamera/Inputs/ZoomCamera"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		DragCameraFinder(TEXT("/OpenRTSCamera/Inputs/DragCamera"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>
		InputMappingContextFinder(TEXT("/OpenRTSCamera/Inputs/OpenRTSCameraInputs"));

	MoveCameraXAxis = MoveCameraXAxisFinder.Object;
	MoveCameraYAxis = MoveCameraYAxisFinder.Object;
	TurnCameraLeft = TurnCameraLeftFinder.Object;
	TurnCameraRight = TurnCameraRightFinder.Object;
	DragCamera = DragCameraFinder.Object;
	ZoomCamera = ZoomCameraFinder.Object;
	InputMappingContext = InputMappingContextFinder.Object;
}

void URTSCamera::BeginPlay()
{
	Super::BeginPlay();
	if (const auto NetMode = GetNetMode() != NM_DedicatedServer)
	{
		/** Populate references we need + setup the desired original position */
		CollectComponentDependencyReferences();
		SetCameraStartingTransform();

		/** Defer ConfigureSpringArm() to the next tick, otherwise we risk slerping our starting position */
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &URTSCamera::ConfigureSpringArm);
		
		ConditionallyEnableEdgeScrolling();
		CheckForEnhancedInputComponent();
		BindInputMappingContext();
		BindInputActions();
	}
}

void URTSCamera::TickComponent(const float DeltaTime,const ELevelTick TickType,	FActorComponentTickFunction* ThisTickFunction)
{	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (const auto NetMode = GetNetMode()!= NM_DedicatedServer && PlayerController->GetViewTarget() == Owner)
	{
		DeltaSeconds = DeltaTime;
		ApplyMoveCameraCommands();
		ConditionallyPerformEdgeScrolling();
		ConditionallyKeepCameraAtDesiredZoomAboveGround();
		SmoothTargetArmLengthToDesiredZoom();
		FollowTargetIfSet();
		ConditionallyApplyCameraBounds();
	}
}

void URTSCamera::FollowTarget(AActor* Target)
{
	CameraFollowTarget = Target;
}

void URTSCamera::UnFollowTarget()
{
	CameraFollowTarget = nullptr;
}

void URTSCamera::SetCameraZoom(const float NewZoomDistance , const bool bSmoothLerp)
{
	if (SpringArm)
	{
		if (bSmoothLerp) OnZoomCamera(NewZoomDistance);
		else SpringArm->TargetArmLength = NewZoomDistance;
	}
}

void URTSCamera::OnZoomCamera(const FInputActionValue& Value)
{
	DesiredZoomLength = FMath::Clamp(DesiredZoomLength + Value.Get<float>() * ZoomSpeed,MinimumZoomLength,MaximumZoomLength);
}

void URTSCamera::OnRotateCameraLeft(const FInputActionValue& Value)
{
	const auto WorldRotation = Root->GetComponentRotation();
	Root->SetWorldRotation(FRotator::MakeFromEuler(FVector(WorldRotation.Euler().X,	WorldRotation.Euler().Y,WorldRotation.Euler().Z -  Value.Get<float>())));
}

void URTSCamera::OnRotateCameraRight(const FInputActionValue& Value)
{
	const auto WorldRotation = Root->GetComponentRotation();
	Root->SetWorldRotation(FRotator::MakeFromEuler(FVector(WorldRotation.Euler().X,	WorldRotation.Euler().Y,WorldRotation.Euler().Z +  Value.Get<float>())));
}

void URTSCamera::OnTurnCameraLeft(const FInputActionValue& Value)
{
	const auto WorldRotation = Root->GetRelativeRotation();
	Root->SetRelativeRotation(FRotator::MakeFromEuler(FVector(WorldRotation.Euler().X,WorldRotation.Euler().Y,WorldRotation.Euler().Z - RotateAngle)));	
}

void URTSCamera::OnTurnCameraRight(const FInputActionValue& Value)
{
	const auto WorldRotation = Root->GetRelativeRotation();
	Root->SetRelativeRotation(FRotator::MakeFromEuler(FVector(WorldRotation.Euler().X,WorldRotation.Euler().Y,WorldRotation.Euler().Z + RotateAngle)));	
}

void URTSCamera::OnMoveCameraYAxis(const FInputActionValue& Value)
{
	RequestMoveCamera(
		SpringArm->GetForwardVector().X,
		SpringArm->GetForwardVector().Y,
		Value.Get<float>()
	);
}

void URTSCamera::OnMoveCameraXAxis(const FInputActionValue& Value)
{
	RequestMoveCamera(
		SpringArm->GetRightVector().X,
		SpringArm->GetRightVector().Y,
		Value.Get<float>()
	);
}

void URTSCamera::OnDragCamera(const FInputActionValue& Value)
{
	if (!IsDragging && Value.Get<bool>())
	{
		IsDragging = true;
		DragStartLocation = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	}

	else if (IsDragging && Value.Get<bool>())
	{
		const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
		auto DragExtents = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
		DragExtents *= DragExtent;

		auto Delta = MousePosition - DragStartLocation;
		Delta.X = FMath::Clamp(Delta.X, -DragExtents.X, DragExtents.X) / DragExtents.X;
		Delta.Y = FMath::Clamp(Delta.Y, -DragExtents.Y, DragExtents.Y) / DragExtents.Y;

		RequestMoveCamera(
			SpringArm->GetRightVector().X,
			SpringArm->GetRightVector().Y,
			Delta.X
		);

		RequestMoveCamera(
			SpringArm->GetForwardVector().X,
			SpringArm->GetForwardVector().Y,
			Delta.Y * -1
		);
	}

	else if (IsDragging && !Value.Get<bool>())
	{
		IsDragging = false;
	}
}

void URTSCamera::RequestMoveCamera(const float X, const float Y, const float Scale)
{
	FMoveCameraCommand MoveCameraCommand;
	MoveCameraCommand.X = X;
	MoveCameraCommand.Y = Y;
	MoveCameraCommand.Scale = Scale;
	MoveCameraCommands.Push(MoveCameraCommand);
}

void URTSCamera::ApplyMoveCameraCommands()
{
	for (const auto& [X, Y, Scale] : MoveCameraCommands)
	{
		auto Movement = FVector2D(X, Y);
		Movement.Normalize();
		Movement *= MoveSpeed * Scale * DeltaSeconds;
		Root->SetWorldLocation(Root->GetComponentLocation() + FVector(Movement.X, Movement.Y, 0.0f));
	}

	MoveCameraCommands.Empty();
}

void URTSCamera::CollectComponentDependencyReferences()
{
	Owner = GetOwner();
	Root = Owner->GetRootComponent();
	Camera = Cast<UCameraComponent>(Owner->GetComponentByClass(UCameraComponent::StaticClass()));
	SpringArm = Cast<USpringArmComponent>(Owner->GetComponentByClass(USpringArmComponent::StaticClass()));
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	TryToFindBoundaryVolumeReference();
}

void URTSCamera::SetCameraStartingTransform()
{
	if (SpringArm)
	{
		/** We disable any lag on the springarm otherwise we will always slow lerp to the desired starting position from rel. 0,0,0 which is undersider behaviour **/
		SpringArm->bEnableCameraLag = false;
		SpringArm->bEnableCameraRotationLag = false;
		SpringArm->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0,StartingYAngle,StartingZAngle)));

		DesiredZoomLength = StartingLenght;
		SetCameraZoom(DesiredZoomLength, false);
	}
}

void URTSCamera::ConfigureSpringArm()
{
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = EnableCameraLag;
	SpringArm->bEnableCameraRotationLag = EnableCameraRotationLag;
}

void URTSCamera::TryToFindBoundaryVolumeReference()
{
	TArray<AActor*> BlockingVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),ACameraBlockingVolume::StaticClass(),BlockingVolumes);

	if (BlockingVolumes.Num() > 0)
	{
		BoundaryVolume = BlockingVolumes[0];
	}
}

void URTSCamera::ConditionallyEnableEdgeScrolling() const
{
	if (EnableEdgeScrolling)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
}

void URTSCamera::CheckForEnhancedInputComponent() const
{
	if (Cast<UEnhancedInputComponent>(PlayerController->InputComponent) == nullptr)
	{
		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Set Edit > Project Settings > Input > Default Classes to Enhanced Input Classes"), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Keyboard inputs will probably not function."), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			GetWorld(),
			TEXT("Error: Enhanced input component not found."), true, true,
			FLinearColor::Red,
			100
		);
	}
}

void URTSCamera::BindInputMappingContext() const
{
	if (PlayerController && PlayerController->GetLocalPlayer())
	{
		if (const auto Input = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			PlayerController->bShowMouseCursor = true;

			// Check if the context is already bound to prevent double binding
			if (!Input->HasMappingContext(InputMappingContext))
			{
				Input->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}

void URTSCamera::BindInputActions()
{
	if (const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
	{
		EnhancedInputComponent->BindAction(ZoomCamera, ETriggerEvent::Triggered,this,&URTSCamera::OnZoomCamera);
		EnhancedInputComponent->BindAction(TurnCameraLeft,ETriggerEvent::Triggered,this,&URTSCamera::OnRotateCameraLeft);
		EnhancedInputComponent->BindAction(TurnCameraRight,ETriggerEvent::Triggered,this,&URTSCamera::OnRotateCameraRight);

		if (bUseIncrementalRotation)
		{
			EnhancedInputComponent->BindAction(TurnCameraLeft,ETriggerEvent::Canceled,this,	&URTSCamera::OnTurnCameraLeft);
			EnhancedInputComponent->BindAction(TurnCameraRight,ETriggerEvent::Canceled,this,&URTSCamera::OnTurnCameraRight);
		}

		EnhancedInputComponent->BindAction(MoveCameraXAxis,ETriggerEvent::Triggered,this,&URTSCamera::OnMoveCameraXAxis);
		EnhancedInputComponent->BindAction(MoveCameraYAxis,ETriggerEvent::Triggered,this,&URTSCamera::OnMoveCameraYAxis);
		EnhancedInputComponent->BindAction(DragCamera,ETriggerEvent::Triggered,this,&URTSCamera::OnDragCamera);
	}
}

void URTSCamera::SetActiveCamera() const
{
	PlayerController->SetViewTarget(GetOwner());
}

void URTSCamera::JumpTo(const FVector Position) const
{
	Root->SetWorldLocation(Position);
}

void URTSCamera::JumpTo(const AActor* Actor) const
{
	Root->SetWorldLocation(Actor->GetActorLocation());
}

void URTSCamera::ConditionallyPerformEdgeScrolling() const
{
	if (EnableEdgeScrolling && !IsDragging)
	{
		EdgeScrollLeft();
		EdgeScrollRight();
		EdgeScrollUp();
		EdgeScrollDown();
	}
}

void URTSCamera::EdgeScrollLeft() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = 1 - UKismetMathLibrary::NormalizeToRange(MousePosition.X,0.0f,ViewportSize.X * DistanceFromEdgeThreshold);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(-1 * Root->GetRightVector() * Movement * EdgeScrollSpeed * DeltaSeconds);
}

void URTSCamera::EdgeScrollRight() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(MousePosition.X,ViewportSize.X * (1 - DistanceFromEdgeThreshold),	ViewportSize.X	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(Root->GetRightVector() * Movement * EdgeScrollSpeed * DeltaSeconds);
}

void URTSCamera::EdgeScrollUp() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(MousePosition.Y,	0.0f,ViewportSize.Y * DistanceFromEdgeThreshold);

	const auto Movement = 1 - UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(Root->GetForwardVector() * Movement * EdgeScrollSpeed * DeltaSeconds);
}

void URTSCamera::EdgeScrollDown() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(MousePosition.Y,ViewportSize.Y * (1 - DistanceFromEdgeThreshold),ViewportSize.Y);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	Root->AddRelativeLocation(-1 * Root->GetForwardVector() * Movement * EdgeScrollSpeed * DeltaSeconds);
}


void URTSCamera::FollowTargetIfSet() const
{
	if (CameraFollowTarget != nullptr)
	{
		Root->SetWorldLocation(CameraFollowTarget->GetActorLocation());
	}
}

void URTSCamera::SmoothTargetArmLengthToDesiredZoom() const
{
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength,DesiredZoomLength,DeltaSeconds,ZoomCatchupSpeed);
}

void URTSCamera::ConditionallyKeepCameraAtDesiredZoomAboveGround()
{
	if (EnableDynamicCameraHeight)
	{
		const auto RootWorldLocation = Root->GetComponentLocation();
		const TArray<AActor*> ActorsToIgnore;

		FHitResult HitResult;
		// Define the object types to detect (Terrain)
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel2)); // Adjust to your "Terrain" channel


		const bool bDidHit = UKismetSystemLibrary::LineTraceSingleForObjects(
			GetWorld(),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z + FindGroundTraceLength),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z - FindGroundTraceLength),
			ObjectTypes,
			true,  // Trace complex
			ActorsToIgnore,
			EDrawDebugTrace::ForOneFrame,
			HitResult,
			true   // Ignore self
		);
		
		if (bDidHit)
		{
			const FVector TargetLocation = FVector(HitResult.Location.X, HitResult.Location.Y, HitResult.Location.Z);
			const FVector SmoothedLocation = FMath::VInterpTo(RootWorldLocation, TargetLocation, DeltaSeconds, ZoomCatchupSpeed);
			Root->SetWorldLocation(SmoothedLocation);
		}
	}
}

void URTSCamera::ConditionallyApplyCameraBounds() const
{
	if (BoundaryVolume != nullptr)
	{
		const auto RootWorldLocation = Root->GetComponentLocation();
		FVector Origin;
		FVector Extents;
		BoundaryVolume->GetActorBounds(false, Origin, Extents);
		Root->SetWorldLocation(FVector(
				UKismetMathLibrary::Clamp(RootWorldLocation.X, Origin.X - Extents.X, Origin.X + Extents.X),
				UKismetMathLibrary::Clamp(RootWorldLocation.Y, Origin.Y - Extents.Y, Origin.Y + Extents.Y),
				RootWorldLocation.Z
			)
		);
	}
}
