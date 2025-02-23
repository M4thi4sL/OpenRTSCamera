/** Copyright 2024 Jesus Bracho All Rights Reserved.  */

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Components/ActorComponent.h"
#include "RTSSelector.generated.h"

class IRTSSelection;
class ARTSHUD;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OPENRTSCAMERA_API URTSSelector : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSSelector();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorsSelected, const TArray<AActor*>&, SelectedActors);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "RTSCamera")
	FOnActorsSelected OnActorsSelected;

	/** BlueprintReadWrite allows access and modification in Blueprints */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* BeginSelection;

	/** Function to clear selected actors, can be overridden in Blueprints  */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void ClearSelectedActors();

	/** Function to handle selected actors, can be overridden in Blueprints */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void HandleSelectedActors(const TArray<AActor*>& NewSelectedActors);
	
	/** BlueprintCallable to allow calling from Blueprints */
	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionStart(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnUpdateSelection(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionEnd(const FInputActionValue& Value);

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Selection")
	TArray<AActor*> SelectedActors;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);

private:
	UPROPERTY()
	APlayerController* PlayerController;

	UPROPERTY()
	ARTSHUD* HUD;

	FVector2D SelectionStart = FVector2d();
	FVector2D SelectionEnd = FVector2d();

	bool bIsSelecting;

	void BindInputActions();
	void BindInputMappingContext() const;
	void CollectComponentDependencyReferences();
};
