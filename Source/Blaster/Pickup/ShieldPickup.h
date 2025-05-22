// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

UCLASS()
class BLASTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Shield Pickup Properties")
	int32 ShieldAmount = 100.f;

	UPROPERTY(EditAnywhere, Category = "Shield Pickup Properties")
	float ShieldReplenishTime = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Shield Pickup Properties")
	float BuffDuration = 30.f;
};
