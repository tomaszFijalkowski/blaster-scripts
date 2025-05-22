// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	void StopGrenade();
	void StartDetonateTimer();
	void DetonateTimerFinished();

	void ExplodeGrenade(const FVector& HitLocation = FVector::ZeroVector, bool bHitPlayer = false);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeGrenade(FVector_NetQuantize HitLocation, bool bHitPlayer);

	UPROPERTY()
	bool bAlreadyExploded = false;

	UPROPERTY(EditAnywhere, Category = "Grenade Properties")
	TObjectPtr<USoundBase> BounceSound;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BounceSoundComponent;

	UPROPERTY(EditAnywhere, Category = "Grenade Properties")
	TObjectPtr<USoundAttenuation> BounceSoundAttenuation;

	UPROPERTY(EditAnywhere, Category = "Grenade Properties")
	float BounceVelocityThreshold = 150.f;

	UPROPERTY(EditAnywhere, Category = "Grenade Properties")
	bool bShouldDetonateOnImpact = true;

	UPROPERTY(EditAnywhere, Category = "Grenade Properties")
	float DetonateTime = 2.5f;

	FTimerHandle DetonateTimer;
};
