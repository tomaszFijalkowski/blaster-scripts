// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ThrowingGrenadePickup.generated.h"

UCLASS()
class BLASTER_API AThrowingGrenadePickup : public APickup
{
	GENERATED_BODY()

public:
	AThrowingGrenadePickup();

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Throwing Grenade Pickup Properties")
	int32 Amount = 1;

	UPROPERTY(EditAnywhere, Category = "Throwing Grenade Pickup Properties", meta = (ClampMin = "0", ClampMax = "255"))
	int32 ThrowingGrenadeCustomDepthStencilValue = 0;
};
