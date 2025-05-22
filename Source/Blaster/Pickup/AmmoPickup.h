// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

UCLASS()
class BLASTER_API AAmmoPickup : public APickup
{
	GENERATED_BODY()

public:
	AAmmoPickup();

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Ammo Pickup Properties")
	int32 AmmoAmount = 30;

	UPROPERTY(EditAnywhere, Category = "Ammo Pickup Properties")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Ammo Pickup Properties", meta = (ClampMin = "0", ClampMax = "255"))
	int32 AmmoCustomDepthStencilValue = 0;
};
