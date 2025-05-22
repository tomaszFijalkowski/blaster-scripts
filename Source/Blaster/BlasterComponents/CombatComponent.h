// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

#define FIRE_DELAY_ERROR_TOLERANCE 0.001f
#define SWITCH_ATTACHED_WEAPONS_DELAY 0.35f
#define SWEEP_SPHERE_RADIUS 5.f

class AFlag;
class UNiagaraComponent;
class UNiagaraSystem;
class AProjectile;
enum class EWeaponType : uint8;
class ABlasterHUD;
class ABlasterPlayerController;
class AWeapon;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* WeaponToEquip);

	void PickupThrowingGrenades(int32 Amount);
	bool HasMaxThrowingGrenades() const;

	UFUNCTION(BlueprintCallable)
	void LaunchThrowingGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchThrowingGrenade(const FVector_NetQuantize& Target);

	UFUNCTION(BlueprintCallable)
	void SwitchAttachedWeapons();

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	bool IsCarriedAmmoFull(EWeaponType WeaponType);

	void RecalculateMovementSpeed();

protected:
	virtual void BeginPlay() override;

	void SetHUDCrosshair(float DeltaTime);

	void AimButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerAimButtonPressed(bool bPressed);

	void FireButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon() const;

	void HandleEquippedWeaponReplication();
	void HandleSecondaryWeaponReplication() const;

	void TraceUnderCrosshair(FHitResult& TraceHitResult);
	float CalculateCrosshairSpread(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReloadMontage();
	int32 AmountToReload();

	void EquipFlag(AFlag* FlagToEquip);
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	void InterpFOV(float DeltaTime);
	void SetSniperScope();
	void SetReloadProgress();

	void Fire();
	void FireProjectileWeapon();
	void FireHitscanWeapon();
	void FireShotgunWeapon();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;
	bool CheckIfWeaponObstructed();

	void Reload();
	void ReloadFinished();
	void UpdateAmmoValues();

	void ThrowGrenade();
	void ThrowGrenadeFinished();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void SwitchWeapons();
	void SwitchWeaponsFinished();

	UPROPERTY(EditAnywhere, Category = "Throwing Grenade")
	TSubclassOf<AProjectile> ThrowingGrenadeClass;

	UPROPERTY(EditAnywhere, Category = "Throwing Grenade")
	float ThrowingGrenadeLaunchAngle = 15.f;

	UPROPERTY(EditAnywhere, Category = "Throwing Grenade")
	int32 MaxThrowingGrenades = 4;

	UPROPERTY(ReplicatedUsing = OnRep_ThrowingGrenades)
	int32 ThrowingGrenades = 1;

	void UpdateHUDThrowingGrenades();

	UFUNCTION()
	void OnRep_ThrowingGrenades();

	FName GetSectionNameByWeaponType() const;

	void DropEquippedWeapon() const;
	void DropSecondaryWeapon() const;
	void DropFlag(bool bForceDrop);
	void AttachActorToSocket(AActor* ActorToAttach, FName SocketName) const;
	void UpdateCarriedAmmo(bool bUpdateHUD = true);
	void ReloadEmptyWeapon();
	void ClearReloadEmptyWeaponTimer();
	void ShowAttachedGrenade(bool bShowGrenade) const;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<ABlasterHUD> HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming = false;

	bool bIsLocallyAiming = false;

	UFUNCTION()
	void OnRep_IsAiming();

	float CalculateAimAccuracyFactor() const;

	bool bIsFiring = false;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 700.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;

	UPROPERTY(EditAnywhere)
	float FlagWalkSpeed = 450.f;

	UPROPERTY(EditAnywhere)
	float CrouchWalkSpeed = 350.f;

	UPROPERTY(EditAnywhere)
	float TraceStartOffset = 25.f;

	UPROPERTY(EditAnywhere)
	FColor CrosshairOverlapColor = FColor::FromHex("FF8989FF");

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	float DefaultFOV;
	float CurrentFOV;

	float DefaultFStop;
	float CurrentFStop;

	float DefaultFocalDistance;
	float CurrentFocalDistance;

	bool bIsSniperScopeVisible = false;
	bool bCanFire = true;

	FTimerHandle FireTimer;

	// Carried ammo for the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	void HandleThrowGrenadeMontage();
	void HandleSwitchWeaponsMontage();

	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, int32> MaxCarriedAmmoMap;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	FTimerHandle ReloadMontageTimer;
	FTimerHandle ThrowGrenadeMontageTimer;
	FTimerHandle SwitchWeaponsTimer;
	FTimerHandle SwitchWeaponsMontageTimer;
	FTimerHandle ShowAttachedGrenadeDelayTimer;
	FTimerHandle FABRIKBlendInTimer;
	FTimerHandle FABRIKBlendOutTimer;

	UPROPERTY(EditAnywhere)
	float ShowAttachedGrenadeDelay = 0.18f;

	// % of the reload duration to blend in FABRIK

	UPROPERTY(EditAnywhere, DisplayName = "FABRIK Reload Blend Percent")
	float FABRIKReloadBlendPercent = 0.09f;

	UPROPERTY(EditAnywhere, DisplayName = "FABRIK Throw Grenade Blend Percent")
	float FABRIKThrowGrenadeBlendPercent = 0.18f;

	UPROPERTY(EditAnywhere, DisplayName = "FABRIK Switch Weapons Blend Percent")
	float FABRIKSwitchWeaponsBlendPercent = 0.18f;

	UPROPERTY()
	float DamageMultiplier = 1.f;

	UPROPERTY()
	float SpeedMultiplier = 1.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsHoldingFlag)
	bool bIsHoldingFlag = false;

	UFUNCTION()
	void OnRep_IsHoldingFlag();

	UPROPERTY(Replicated)
	TObjectPtr<AFlag> Flag;

	UPROPERTY()
	float ReloadStartTime = 0.f;

	UPROPERTY()
	float ReloadProgress = 0.f;

	UPROPERTY()
	float ReloadDuration = 0.f;

	UPROPERTY(EditAnywhere)
	float ReloadEmptyWeaponDelay = 0.3f;

	FTimerHandle ReloadEmptyWeaponTimer;

	bool bAwaitingEmptyReload = false;

public:
	FORCEINLINE float GetAimWalkSpeed() const { return AimWalkSpeed; }
	FORCEINLINE void SetAimWalkSpeed(const float Speed) { AimWalkSpeed = Speed; }
	FORCEINLINE float GetBaseWalkSpeed() const { return BaseWalkSpeed; }
	FORCEINLINE void SetBaseWalkSpeed(const float Speed) { BaseWalkSpeed = Speed; }
	FORCEINLINE int32 GetThrowingGrenades() const { return ThrowingGrenades; }
	FORCEINLINE float GetDamageMultiplier() const { return DamageMultiplier; }
	FORCEINLINE void SetDamageMultiplier(const float Multiplier) { DamageMultiplier = Multiplier; }
	FORCEINLINE float GetSpeedMultiplier() const { return SpeedMultiplier; }
	FORCEINLINE void SetSpeedMultiplier(const float Multiplier) { SpeedMultiplier = Multiplier; }
	FORCEINLINE float GetReloadProgress() const { return ReloadProgress; }
	FORCEINLINE float GetReloadDuration() const { return ReloadDuration; }
};
