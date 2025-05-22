// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitscanWeapon.generated.h"

#define LINE_TRACE_MULTIPLIER 1.25f
#define SOUND_2D_VOLUME_MULTIPLIER 1.25f

UCLASS()
class BLASTER_API AHitscanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	virtual void SetImpactEffects(EPhysicalSurface PhysicalSurface, bool bIsHeadshot);
	virtual void PlayImpactEffects(const FHitResult& FireHit, const bool bPlaySound = true) const;
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit) const;

private:
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> BeamParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> MetalImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> StoneImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> WoodImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> PlayerImpactParticles;

	UPROPERTY()
	TObjectPtr<UParticleSystem> ImpactParticlesToSpawn;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> DefaultImpactSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> BodyshotImpactSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> HeadshotImpactSound;

	UPROPERTY()
	TObjectPtr<USoundBase> ImpactSoundToPlay;
};
