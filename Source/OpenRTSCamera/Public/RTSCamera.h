// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RTSCamera.generated.h"

/**
 * We use these commands so that move camera inputs can be tied to the tick rate of the game.
 * https://github.com/HeyZoos/OpenRTSCamera/issues/27
 */
USTRUCT()
struct FMoveCameraCommand
{
	GENERATED_BODY()
	UPROPERTY()
	float X = 0;
	UPROPERTY()
	float Y = 0;
	UPROPERTY()
	float Scale = 0;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OPENRTSCAMERA_API URTSCamera : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSCamera();

	/** Tells the camera to follow a certain actor.
	 * @param Target - What actor to follow.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void FollowTarget(AActor* Target);

	/** Stop following a target, if target is valid */
	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void UnFollowTarget();

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void SetActiveCamera() const;

	/** Slerps the camera position to the given position
	 * @param Position - The position we want to slerp towards */
	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void JumpTo(const FVector Position) const;
	void JumpTo(const AActor* Actor) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|ZoomSettings")
	float MinimumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|ZoomSettings")
	float MaximumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|ZoomSettings")
	float ZoomCatchupSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|ZoomSettings")
	float ZoomSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingYAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingZAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MoveSpeed;

	/** Should we allow rotating by an incremental value upon tapping the hit button?*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Rotation")
	bool bUseIncrementalRotation = false;
	
	/** In what incremental degree should we rotate the camera? */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Rotation", meta = (EditCondition = "bUseIncrementalRotation"))
	float RotateAngle;
	
	/**
	 * Controls how fast the drag will move the camera.
	 * Higher values will make the camera move more slowly.
	 * The drag speed is calculated as follows:
	 *	DragSpeed = MousePositionDelta / (ViewportExtents * DragExtent)
	 * If the drag extent is small, the drag speed will hit the "max speed" of `this->MoveSpeed` more quickly.
	 */
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "RTSCamera",meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DragExtent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraLag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraRotationLag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|DynamicCameraHeightSettings")
	bool EnableDynamicCameraHeight;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|DynamicCameraHeightSettings",meta=(EditCondition="EnableDynamicCameraHeight"))
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "RTSCamera|DynamicCameraHeightSettings",meta=(EditCondition="EnableDynamicCameraHeight"))
	float FindGroundTraceLength;

	/** Should the camera support edgescrolling behaviour? */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|EdgeScrollSettings")
	bool EnableEdgeScrolling;
	
	/** Attempts to keep the viewport within the bounds of the blocking volume. So that the viewport never extends out of the desired volume */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|EdgeScrollSettings",meta=(EditCondition="EnableEdgeScrolling"))
	bool bKeepViewportWithinBounds = true;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "RTSCamera|EdgeScrollSettings",meta=(EditCondition="EnableEdgeScrolling"))
	float EdgeScrollSpeed;

	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "RTSCamera|EdgeScrollSettings",meta=(EditCondition="EnableEdgeScrolling"))
	float DistanceFromEdgeThreshold;

	/** Input actions */
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputMappingContext* InputMappingContext;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* RotateCameraLeft;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* RotateCameraRight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* TurnCameraLeft;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* TurnCameraRight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* MoveCameraYAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* MoveCameraXAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* DragCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera|Inputs")
	UInputAction* ZoomCamera;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;

	void OnZoomCamera(const FInputActionValue& Value);
	void OnRotateCameraLeft(const FInputActionValue& Value);
	void OnRotateCameraRight(const FInputActionValue& Value);
	void OnTurnCameraLeft(const FInputActionValue& Value);
	void OnTurnCameraRight(const FInputActionValue& Value);
	void OnMoveCameraYAxis(const FInputActionValue& Value);
	void OnMoveCameraXAxis(const FInputActionValue& Value);
	void OnDragCamera(const FInputActionValue& Value);

	void RequestMoveCamera(float X, float Y, float Scale);
	void ApplyMoveCameraCommands();

	UPROPERTY()
	AActor* Owner;
	UPROPERTY()
	USceneComponent* Root;
	UPROPERTY()
	UCameraComponent* Camera;
	UPROPERTY()
	USpringArmComponent* SpringArm;
	UPROPERTY()
	APlayerController* PlayerController;
	UPROPERTY()
	AActor* BoundaryVolume;
	UPROPERTY()
	float DesiredZoomLength;

private:
	void CollectComponentDependencyReferences();
	void ConfigureSpringArm();
	void TryToFindBoundaryVolumeReference();
	void ConditionallyEnableEdgeScrolling() const;
	void CheckForEnhancedInputComponent() const;
	void BindInputMappingContext() const;
	void BindInputActions();

	void ConditionallyPerformEdgeScrolling() const;
	void EdgeScrollLeft() const;
	void EdgeScrollRight() const;
	void EdgeScrollUp() const;
	void EdgeScrollDown() const;

	void FollowTargetIfSet() const;
	void SmoothTargetArmLengthToDesiredZoom() const;
	void ConditionallyKeepCameraAtDesiredZoomAboveGround();
	void ConditionallyApplyCameraBounds() const;

	UPROPERTY()
	FName CameraBlockingVolumeTag;
	
	UPROPERTY()
	AActor* CameraFollowTarget;
	
	UPROPERTY()
	float DeltaSeconds;

	
	UPROPERTY()
	bool IsDragging;
	
	UPROPERTY()
	FVector2D DragStartLocation;
	
	UPROPERTY()
	TArray<FMoveCameraCommand> MoveCameraCommands;
};
