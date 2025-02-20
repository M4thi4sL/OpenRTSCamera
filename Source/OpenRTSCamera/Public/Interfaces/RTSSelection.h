// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSSelection.generated.h"

// Declaring the interface
UINTERFACE(MinimalAPI, Blueprintable)
class URTSSelection : public UInterface
{
	GENERATED_BODY()
};

/**
 * C++ interface, any actor that implements this function is considered selectable.
 */
class OPENRTSCAMERA_API IRTSSelection
{
	GENERATED_BODY()

public:
	/** Called when the actor is selected */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS Selection")
	void OnSelected();

	/** Called when the actor is deselected */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS Selection")
	void OnDeselected();
};
	


