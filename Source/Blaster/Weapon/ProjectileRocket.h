// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void BeginPlay() override;

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void HideProjectile() override;

private:
	void ExplodeRocket(const FVector& HitLocation = FVector::ZeroVector, bool bHitPlayer = false);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeRocket(FVector_NetQuantize HitLocation, bool bHitPlayer);

	UPROPERTY()
	bool bAlreadyExploded = false;

	UPROPERTY(EditAnywhere, Category = "Rocket Properties")
	TObjectPtr<USoundBase> ProjectileLoopSound;

	UPROPERTY()
	TObjectPtr<UAudioComponent> ProjectileLoopSoundComponent;

	UPROPERTY(EditAnywhere, Category = "Rocket Properties")
	TObjectPtr<USoundAttenuation> ProjectileLoopSoundAttenuation;

	UPROPERTY(VisibleAnywhere, Category = "Rocket Properties")
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;
};
