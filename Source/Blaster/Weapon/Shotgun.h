// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitscanWeapon.h"
#include "Shotgun.generated.h"

USTRUCT()
struct FFireHit
{
	GENERATED_BODY()

public:
	FHitResult HitResult;
	bool bIsHeadshot;
};

UCLASS()
class BLASTER_API AShotgun : public AHitscanWeapon
{
	GENERATED_BODY()

public:
	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutTraceEnds,
	                                bool bIsAiming, float AimAccuracyFactor) const;

protected:
	virtual void SetImpactEffects(EPhysicalSurface PhysicalSurface, bool bIsHeadshot) override;
	virtual void PlayImpactEffects(const FHitResult& FireHit, const bool bPlaySound = true) const override;

private:
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float MinImpactDistance = 100.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 MaxNearbyImpacts = 1;

	UPROPERTY(EditAnywhere, Category = "Combat")
	uint32 NumberOfPellets = 8;

	UPROPERTY()
	TArray<FFireHit> FireHits;

	UPROPERTY()
	TArray<FVector> ProcessedImpactPoints;
};
