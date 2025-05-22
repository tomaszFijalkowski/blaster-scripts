// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

UCLASS()
class BLASTER_API AJumpPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Jump Pickup Properties")
	float JumpMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Jump Pickup Properties")
	float BuffDuration = 30.f;
};
