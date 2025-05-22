// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

enum class EBuffType : uint8;
class UNiagaraSystem;
class ABlasterCharacter;

#define MAX_BUFF_DURATION_ENTRIES 4
#define MIN_BUFF_REMAINING_TIME 0.2f // To avoid handling entries that are about to expire and avoid flickering

USTRUCT(BlueprintType)
struct FBuffDurationEntry
{
	GENERATED_BODY()

public:
	EBuffType BuffType;
	float Duration;
	float RemainingTime;
	FLinearColor FillColor;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);

	void BuffShield(float ShieldAmount, float ShieldReplenishTime, float BuffDuration);
	void DepleteShield();

	void BuffSpeed(float SpeedMultiplier, float BuffDuration);

	void BuffJump(float JumpMultiplier, float BuffDuration);
	void SetInitialJumpVelocity(float JumpVelocity);

	void BuffDamage(float DamageMultiplier, float BuffDuration);

protected:
	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);
	void UpdateBuffRemainingTimes(float DeltaTime);

private:
	void SetBuffDurationEntries();

	UPROPERTY()
	TArray<FBuffDurationEntry> BuffDurationEntries;

	UPROPERTY()
	TMap<EBuffType, int32> BuffTypeToIndexMap; // Allows quick lookup of BuffDurationEntries index by BuffType

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	/**
	 * Heal buff
	 */
	bool bIsHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/**
	 * Shield buff
	 */
	UPROPERTY(EditAnywhere, Category = "Shield Buff Properties")
	TObjectPtr<UNiagaraSystem> ShieldBuffEffect;

	UPROPERTY(EditAnywhere, Category = "Shield Buff Properties")
	FLinearColor ShieldBuffFillColor;

	void ResetShield();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuffShield(float BuffDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetShield();

	void HandleBuffShield(float BuffDuration);
	void HandleResetShield();

	bool bIsReplenishingShield = false;
	float ShieldReplenishRate = 0.f;
	float ShieldToReplenish = 0.f;

	FTimerHandle ShieldBuffTimer;

	float ShieldBuffDuration = 0.f;
	float ShieldBuffRemainingTime = 0.f;

	/**
	 * Speed buff
	 */
	UPROPERTY(EditAnywhere, Category = "Speed Buff Properties")
	FLinearColor SpeedBuffFillColor;

	void ResetSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuffSpeed(float SpeedMultiplier, float BuffDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetSpeed();

	void HandleBuffSpeed(float SpeedMultiplier, float BuffDuration);
	void HandleResetSpeed();

	FTimerHandle SpeedBuffTimer;

	float SpeedBuffDuration = 0.f;
	float SpeedBuffRemainingTime = 0.f;

	/**
	 * Jump buff
	 */
	UPROPERTY(EditAnywhere, Category = "Jump Buff Properties")
	FLinearColor JumpBuffFillColor;

	UPROPERTY(EditAnywhere, Category = "Jump Buff Properties")
	TObjectPtr<USoundBase> JumpSound;

	UPROPERTY(EditAnywhere, Category = "Jump Buff Properties")
	TObjectPtr<USoundAttenuation> JumpSoundAttenuation;

	void ResetJumpVelocity();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuffJump(float JumpMultiplier, float BuffDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetJumpVelocity();

	void HandleBuffJump(float JumpMultiplier, float BuffDuration);
	void HandleResetJumpVelocity();

	float InitialJumpVelocity;

	FTimerHandle JumpBuffTimer;

	float JumpBuffDuration = 0.f;
	float JumpBuffRemainingTime = 0.f;

	/**
	 * Damage buff
	 */
	UPROPERTY(EditAnywhere, Category = "Damage Buff Properties")
	FLinearColor DamageBuffFillColor;

	UPROPERTY(EditAnywhere, Category = "Damage Buff Properties")
	TObjectPtr<UNiagaraSystem> DamageBuffEffect;

	void ResetDamage();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuffDamage(float DamageMultiplier, float BuffDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetDamage();

	void HandleBuffDamage(float DamageMultiplier, float BuffDuration);
	void HandleResetDamage();

	FTimerHandle DamageBuffTimer;

	float DamageBuffDuration = 0.f;
	float DamageBuffRemainingTime = 0.f;
};
