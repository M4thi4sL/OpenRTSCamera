// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSCameraBoundsVolume.h"
#include "Components/PrimitiveComponent.h"

ARTSCameraBoundsVolume::ARTSCameraBoundsVolume()
{
    Tags.Add("OpenRTSCamera#CameraBounds");
    /**
    if (UPrimitiveComponent* PrimitiveComponent = this->FindComponentByClass<UPrimitiveComponent>())
    {
        PrimitiveComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName, false);
    }
    */
}
