// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class ABlasterCharacter;
class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;

	void SetDamageProperties(float InDamage, float InHeadshotDamage, float InMinDamage, float InDamageInnerRadius,
	                         float InDamageOuterRadius);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

	virtual void HideProjectile();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnHit(EPhysicalSurface PhysicalSurface, bool bHitHead);

	bool ExplodeDamage();

	void SpawnTrailSystem();

	void SetImpactEffects(EPhysicalSurface PhysicalSurface, bool bHitHead = false);
	void PlayImpactEffects() const;
	void PlayImpactSound() const;

	void StartDestroyTimer();
	void DestroyTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float InitialSpeed = 15000.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float HeadshotDamage = 30.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float MinDamage = 5.f; // Used for radial damage falloff

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float DamageOuterRadius = 400.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Projectile Properties")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float TrailSystemOffsetX = -13.5f;

	/*
	 * Server-side rewind properties
	 */
	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

private:
	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UParticleSystem> TracerParticles;

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> TracerParticlesComponent;

	bool bShouldHideTracerOnInit = false;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UParticleSystem> MetalImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UParticleSystem> StoneImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UParticleSystem> WoodImpactParticles;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<UParticleSystem> PlayerImpactParticles;

	UPROPERTY()
	TObjectPtr<UParticleSystem> ImpactParticlesToSpawn;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float ImpactParticlesScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<USoundBase> DefaultImpactSound;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<USoundBase> BodyshotImpactSound;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<USoundBase> HeadshotImpactSound;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY()
	TObjectPtr<USoundBase> ImpactSoundToPlay;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties")
	float DestroyTime = 5.f;

	FTimerHandle DestroyTimer;

public:
	FORCEINLINE TObjectPtr<UBoxComponent> GetCollisionBox() const { return CollisionBox; }
	FORCEINLINE TObjectPtr<UStaticMeshComponent> GetMesh() const { return ProjectileMesh; }
	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }
	FORCEINLINE bool GetUseServerSideRewind() const { return bUseServerSideRewind; }
	FORCEINLINE void SetUseServerSideRewind(const bool bUse) { bUseServerSideRewind = bUse; }
	FORCEINLINE void SetTraceStart(const FVector_NetQuantize& Location) { TraceStart = Location; }
	FORCEINLINE void SetInitialVelocity(const FVector_NetQuantize100& Velocity) { InitialVelocity = Velocity; }
};
