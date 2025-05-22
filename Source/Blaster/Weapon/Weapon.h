// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class ABlasterPlayerController;
class ABlasterCharacter;
class AAmmoCasing;
class UWidgetComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickedUp);

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Fire(const FVector& HitTarget);
	virtual bool CanBeDropped(bool bPerformTrace);
	virtual void Dropped();
	virtual void ShowPickupWidget(bool bShowWidget);

	virtual FVector GetPickupSocketLocation() const;
	virtual FVector GetMuzzleFlashSocketLocation() const;

	bool IsFalling() const;

	void SetWeaponState(const EWeaponState State);
	void SetHUDWeaponAmmo();
	void SetHUDCarriedAmmo();
	void PlayPickupSound();
	void AddAmmo(int32 AmmoToAdd);
	void EnableCustomDepth(bool bEnable) const;
	FName GetSecondaryWeaponSocket() const;
	void SpawnDamageBuffEffect(UNiagaraSystem* DamageBuffEffect);
	void SetDamageBuffEffectVisibility(bool bNewVisibility) const;
	float CalculateDamageWithFalloff(float BaseDamage, float Distance) const;
	FVector TraceEndWithScatter(const FVector& HitTarget, bool bIsAiming, float AimAccuracyFactor) const;

	FOnPickedUp OnPickedUp;

protected:
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();

	virtual void HandleWeaponInitialState();
	virtual void HandleWeaponEquippedState();
	virtual void HandleWeaponEquippedSecondaryState();
	virtual void HandleWeaponDroppedState();

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	virtual void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                            FVector NormalImpulse, const FHitResult& Hit);

	virtual void OnRep_Owner() override;

	void SetPickupWidget();
	void SetPickupWidgetComponentPosition() const;

	void PlayFireEffects(const USkeletalMeshSocket* MuzzleFlashSocket) const;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> OwnerController;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidgetComponent;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UUserWidget> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bUseServerSideRewind = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	ETeam Team;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	float Amplitude = 0.125f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	float TimeConstant = 1.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	bool bShouldHover = true;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter", meta = (EditCondition = "bUseScatter"))
	float DistanceToScatterSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter", meta = (EditCondition = "bUseScatter"))
	float ScatterSphereRadius = 60.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter", meta = (EditCondition = "bUseScatter"))
	float AimingScatterSphereRadius = 30.f;

	UPROPERTY()
	bool PendingSpawnDamageBuffEffect = false;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HeadshotDamage = 30.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MinDamage = 5.f; // Used for radial damage falloff

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageOuterRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> DropSound;

	// Should be server-authoritative
	bool bIsFalling = false;

private:
	void CheckForOverlappingActors();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float CheckForOverlappingActorsInterval = 0.1f;

	FTimerHandle CheckForOverlappingActorsTimer;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<AAmmoCasing> AmmoCasingClass;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UParticleSystem> MuzzleFlashParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bEnableWeaponPhysics = false;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom;

	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomedFOV = 45.f;

	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomInSpeed = 18.f;

	UPROPERTY(EditAnywhere, Category = "Zoom")
	float ZoomOutSpeed = 6.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bUseDamageFalloff = false;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "bUseDamageFalloff"))
	float DamageFalloffStartDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat ", meta = (EditCondition = "bUseDamageFalloff"))
	float DamageFalloffEndDistance = 8000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (EditCondition = "bUseDamageFalloff"))
	float MinDamagePercentAfterFalloff = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bIsAutomatic = true;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .125f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 Ammo;

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddAmmo(int32 AmmoToAdd);

	void UpdateOwnersIfNecessary();

	void HandleHovering(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float RunningTime;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MagCapacity;

	// The number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties", meta = (ClampMin = "0", ClampMax = "255"))
	int32 WeaponCustomDepthStencilValue = 0;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	FLinearColor AmmoColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Damage Buff Effect")
	float DamageBuffEffectRollOffset = 0.f;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> DamageBuffEffectComponent = nullptr;

	void StartDestroyWeaponTimer();
	void StopDestroyWeaponTimer();
	void DestroyWeapon();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float DestroyWeaponTime = 30.f;

	FTimerHandle DestroyWeaponTimer;

public:
	FORCEINLINE TObjectPtr<USphereComponent> GetOverlapSphere() const { return OverlapSphere; }
	FORCEINLINE TObjectPtr<UWidgetComponent> GetPickupWidget() const { return PickupWidgetComponent; }
	FORCEINLINE TObjectPtr<USkeletalMeshComponent> GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE TObjectPtr<UTexture2D> GetCrosshairCenter() const { return CrosshairCenter; }
	FORCEINLINE TObjectPtr<UTexture2D> GetCrosshairLeft() const { return CrosshairLeft; }
	FORCEINLINE TObjectPtr<UTexture2D> GetCrosshairRight() const { return CrosshairRight; }
	FORCEINLINE TObjectPtr<UTexture2D> GetCrosshairTop() const { return CrosshairTop; }
	FORCEINLINE TObjectPtr<UTexture2D> GetCrosshairBottom() const { return CrosshairBottom; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInSpeed() const { return ZoomInSpeed; }
	FORCEINLINE float GetZoomOutSpeed() const { return ZoomOutSpeed; }
	FORCEINLINE bool GetIsAutomatic() const { return bIsAutomatic; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; }
	FORCEINLINE int32 GetWeaponCustomDepthStencilValue() const { return WeaponCustomDepthStencilValue; }
	FORCEINLINE FLinearColor GetAmmoColor() const { return AmmoColor; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetFireType() const { return FireType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE bool GetUseScatter() const { return bUseScatter; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadshotDamage() const { return HeadshotDamage; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
