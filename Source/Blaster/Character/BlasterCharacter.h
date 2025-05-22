// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairInterface.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

#define YAW_INTERP_SPEED 4.f
#define VIEW_PITCH_LIMIT 89.f
#define CAPSULE_VISIBILITY_SCALE 0.9f
#define BASE_MOUSE_SENSITIVITY 0.251454f
#define DEFAULT_FOV 90.f

class UBlasterGameUserSettings;
class UBlasterDamageType;
class UDamageWidget;
class UOverheadWidget;
class ABlasterGameMode;
class ULagCompensationComponent;
class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UBuffComponent;
enum class ECombatState : uint8;
class ABlasterPlayerState;
class UTimelineComponent;
class ABlasterPlayerController;
class UCombatComponent;
class AWeapon;
class UWidgetComponent;
struct FInputActionValue;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeaveGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Destroyed() override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void AddOverlappingWeapon(AWeapon* Weapon);
	void RemoveOverlappingWeapon(AWeapon* Weapon);
	void DetermineWeaponToPickup();

	TObjectPtr<AWeapon> GetEquippedWeapon() const;
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsHoldingFlag() const;
	void SetHoldingFlag(bool bHolding) const;
	ETeam GetTeam();

	void PlayFireMontage(bool bIsAiming) const;
	void PlayReloadMontage(FName SectionName) const;
	void StopReloadMontage() const;
	void PlayThrowGrenadeMontage() const;
	void PlaySwitchWeaponsMontage() const;

	void PlayWeaponObstructionSound() const;

	float GetReloadDuration(FName SectionName) const;
	float GetThrowGrenadeDuration() const;
	float GetSwitchWeaponsDuration() const;

	FVector GetHitTarget() const;
	ECombatState GetCombatState() const;

	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

	void HandleCooldown();

	void UpdateHUDHealth() const;
	void UpdateHUDShield() const;
	void UpdateHUDAmmo() const;
	void UpdateHUDReloadProgress() const;
	void UpdateHUDReloadProgressVisibility(bool bVisibility) const;
	void UpdateHUDBuffDurations() const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedMVP();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostMVP();

	void SpawnShieldBuffEffect(UNiagaraSystem* ShieldBuffEffect);

	UFUNCTION(Server, Reliable)
	void ServerInitiateLeavingGame();

	void LeaveGame();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLeaveGame();

	FOnLeaveGame OnLeaveGame;

	void SetTeamColor(ETeam Team);

	UFUNCTION()
	void StopAiming();

	UFUNCTION()
	void StopFiring();

	TObjectPtr<ABlasterPlayerController> GetMostRecentDamager() const;
	TObjectPtr<ABlasterPlayerController> GetSecondMostRecentDamager() const;

protected:
	virtual void BeginPlay() override;

	void PollPlayerStateInit();
	void PollSettingTeamColor();
	void PollSettingOverheadWidget();
	void PollSettingDamageWidget();

	// Enhanced input methods
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void CustomJump(const FInputActionValue& Value);
	void CustomCrouch(const FInputActionValue& Value);
	void Equip(const FInputActionValue& Value);
	void Aim(const FInputActionValue& Value);
	void Fire(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);
	void ThrowGrenade(const FInputActionValue& Value);
	void SwitchWeapons(const FInputActionValue& Value);

	void RotateInPlace(float DeltaTime);
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	void PlayHitReactMontage() const;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	                   AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION(Client, Reliable)
	void ClientShowDamageNumber(float DamageAmount, const UBlasterDamageType* DamageType,
	                            ABlasterCharacter* DamagedCharacter);

	void SetOverheadWidget();
	void SetDamageWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> BlasterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> LookAroundMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ThrowGrenadeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SwitchWeaponsAction;

	/**
	 * Hitboxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> HeadBox; // head

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> PelvisBox; // pelvis

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> Spine1Box; // spine_01

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> Spine2Box; // spine_02

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> Spine3Box; // spine_03

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftUpperArmBox; // upperarm_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightUpperArmBox; // upperarm_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftLowerArmBox; // lowerarm_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightLowerArmBox; // lowerarm_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftHandBox; // hand_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightHandBox; // hand_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftThighBox; // thigh_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightThighBox; // thigh_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftCalfBox; // calf_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightCalfBox; // calf_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> LeftFootBox; // foot_l

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> RightFootBox; // foot_r

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> BackpackBox; // backpack

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TObjectPtr<UBoxComponent> BlanketBox; // blanket

	UPROPERTY()
	TMap<FName, TObjectPtr<UBoxComponent>> Hitboxes;

private:
	void InitializeEnhancedInput() const;

	void SetInitialRotation();
	void SpawnDefaultWeapon();

	void SetShieldEffectPosition() const;
	void SetCrownEffectPosition() const;
	void SetOverheadWidgetPosition() const;
	void SetDamageWidgetPosition() const;

	UFUNCTION()
	void OnRep_WeaponToPickup(AWeapon* PreviousWeaponToPickup) const;

	UFUNCTION(Server, Reliable)
	void ServerEquip();

	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapons();

	float CalculateSpeed() const;
	void CalculateAO_Pitch();
	void TurnInPlace(float DeltaTime);

	void HideCharacterIfCameraTooClose() const;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	void PlayEliminationMontage() const;

	void RespawnTimerFinished();

	void SetHUDRespawnTimer();

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	UFUNCTION()
	void StartDissolve();

	void EnableLookAroundOnly();

	void ApplyDamageCauserMultiplier(float& Damage, const AActor* DamageCauser) const;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float RespawnTime = 2.f;

	void CheckIfInsideSpawnZone();

	bool bIsInsideSpawnZone = false;
	bool bIsEliminated = false;

	FTimerHandle RespawnTimer;
	FTimerHandle HUDRespawnTimer;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<UBlasterGameUserSettings> GameUserSettings;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraThreshold = 175.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY()
	TObjectPtr<UOverheadWidget> OverheadWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetComponent> DamageWidgetComponent;

	UPROPERTY()
	TObjectPtr<UDamageWidget> DamageWidget;

	UPROPERTY(EditAnywhere, Category = "Damage Widget")
	float DamageAccumulationTime = 1.75f;

	float AccumulatedDamage = 0.f;

	bool bIsDamageAccumulating = false;

	FTimerHandle DamageAccumulationTimer;

	UPROPERTY()
	TSet<TObjectPtr<AWeapon>> OverlappingWeapons;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponToPickup)
	TObjectPtr<AWeapon> WeaponToPickup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBuffComponent> BuffComponent;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULagCompensationComponent> LagCompensationComponent;

	float AnimationPlayRate = 1.f;
	float FABRIKBlendTime = 0.15f;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	float DefaultFOV = 90.f;

	float CalculateSensitivityScale() const;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> EliminationMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> SwitchWeaponsMontage;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float TimeSinceLastMovementReplicationThreshold = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInstance> DefaultMaterial;

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInstance> BlueMaterial;

	UPROPERTY(EditAnywhere, Category = "Material")
	TObjectPtr<UMaterialInstance> RedMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<UCurveFloat> DissolveCurve;

	// Dynamic material instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;

	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<UMaterialInstance> DefaultDissolveMaterial;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<UMaterialInstance> BlueDissolveMaterial;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<UMaterialInstance> RedDissolveMaterial;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<UParticleSystem> EliminationBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	TObjectPtr<UParticleSystemComponent> EliminationBotComponent;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	float EliminationBotZOffset = 200.f;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	TObjectPtr<USoundBase> EliminationBotSound;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;

	UPROPERTY(VisibleAnywhere, Category = "Throwing Grenade")
	TObjectPtr<UStaticMeshComponent> AttachedGrenade;

	bool bIsLookAroundOnly = false;
	bool bUseFABRIK = true;

	UPROPERTY(EditAnywhere, Category = "Crown Effect")
	float CrownEffectZOffset = 60.f;

	UPROPERTY(EditAnywhere, Category = "Crown Effect")
	TObjectPtr<UNiagaraSystem> CrownEffect;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> CrownEffectComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ShieldBuffEffectComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> DamageBuffEffect = nullptr;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayJumpSound();

	UFUNCTION(Server, Reliable)
	void ServerPlayJumpSound();

	UPROPERTY()
	TObjectPtr<USoundBase> JumpSound;

	UPROPERTY()
	TObjectPtr<USoundAttenuation> JumpSoundAttenuation;

	UPROPERTY(EditAnywhere, Category = "Character Properties")
	TObjectPtr<USoundBase> WeaponObstructionSound;

	UPROPERTY(EditAnywhere, Category = "Character Properties")
	TObjectPtr<USoundAttenuation> WeaponObstructionSoundAttenuation;

	UPROPERTY(EditAnywhere, Category = "Character Properties")
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> LocallyControlledCharacter;

	ABlasterCharacter* GetLocallyControlledCharacter() const;

	UPROPERTY(EditAnywhere, Category = "Widget")
	TObjectPtr<UCurveFloat> WidgetScaleCurve;

	UFUNCTION()
	void UpdateOverheadWidgetDistance();

	UFUNCTION()
	void UpdateOverheadWidgetVisibility();

	bool bIsWithinOverheadWidgetVisibilityDistance = false;

	UPROPERTY(VisibleAnywhere, Category = "Overhead Widget")
	float OverheadWidgetZOffset = 0.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetZOffsetClose = 225.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetZOffsetFar = 275.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetScaleClose = 1.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetScaleFar = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetOpacityClose = 1.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetOpacityFar = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetVisibilityDistance = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Overhead Widget")
	float OverheadWidgetVisibilityCheckInterval = 0.1f;

	FTimerHandle OverheadWidgetVisibilityTimer;

	UFUNCTION()
	void UpdateDamageWidgetDistance();

	UPROPERTY(VisibleAnywhere, Category = "Damage Widget")
	float DamageWidgetZOffset = 0.f;

	UPROPERTY(EditAnywhere, Category = "Damage Widget")
	float DamageWidgetZOffsetClose = 200.f;

	UPROPERTY(EditAnywhere, Category = "Damage Widget")
	float DamageWidgetZOffsetFar = 220.f;

	UPROPERTY(EditAnywhere, Category = "Damage Widget")
	float DamageWidgetScaleClose = 1.f;

	UPROPERTY(EditAnywhere, Category = "Damage Widget")
	float DamageWidgetScaleFar = 0.4f;

	TMap<TObjectPtr<AController>, float> DamagerTimestamps;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamagerTrackingTime = 8.f;

	void CleanupDamagerHistory();

	bool bTeamColorSet = false;

public:
	FORCEINLINE float GetAnimationPlayRate() const { return AnimationPlayRate; }
	FORCEINLINE void SetAnimationPlayRate(const float PlayRate) { AnimationPlayRate = PlayRate; }
	FORCEINLINE float GetFABRIKBlendTime() const { return FABRIKBlendTime; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE bool GetRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE TObjectPtr<UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(const float HealthAmount) { Health = HealthAmount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE bool IsFullHealth() const { return Health == MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(const float ShieldAmount) { Shield = ShieldAmount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE bool IsFullShield() const { return Shield == MaxShield; }
	FORCEINLINE bool IsInsideSpawnZone() const { return bIsInsideSpawnZone; }
	FORCEINLINE void SetIsInsideSpawnZone(const bool bIsInside) { bIsInsideSpawnZone = bIsInside; }
	FORCEINLINE bool IsEliminated() const { return bIsEliminated; }
	FORCEINLINE bool IsLookAroundOnly() const { return bIsLookAroundOnly; }
	FORCEINLINE bool IsUsingFABRIK() const { return bUseFABRIK; }
	FORCEINLINE void SetUseFABRIK(const bool bUse) { bUseFABRIK = bUse; }
	FORCEINLINE TObjectPtr<UStaticMeshComponent> GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE TObjectPtr<UCombatComponent> GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE TObjectPtr<UBuffComponent> GetBuffComponent() const { return BuffComponent; }
	FORCEINLINE TObjectPtr<ULagCompensationComponent> GetLagCompensationComponent() const
	{
		return LagCompensationComponent;
	}

	FORCEINLINE void SetJumpSound(USoundBase* Sound) { JumpSound = Sound; }
	FORCEINLINE void SetJumpSoundAttenuation(USoundAttenuation* SoundAttenuation)
	{
		JumpSoundAttenuation = SoundAttenuation;
	}

	FORCEINLINE UNiagaraSystem* GetDamageBuffEffect() const { return DamageBuffEffect; }
	FORCEINLINE void SetDamageBuffEffect(UNiagaraSystem* Effect) { DamageBuffEffect = Effect; }
	FORCEINLINE TMap<FName, TObjectPtr<UBoxComponent>> GetHitboxes() const { return Hitboxes; }
};
