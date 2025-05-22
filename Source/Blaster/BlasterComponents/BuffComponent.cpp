// Fill out your copyright notice in the Description page of Project Settings.

#include "BuffComponent.h"
#include "Blaster/BlasterTypes/BuffType.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);

	UpdateBuffRemainingTimes(DeltaTime);
}

void UBuffComponent::Heal(const float HealAmount, const float HealingTime)
{
	bIsHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::BuffShield(const float ShieldAmount, const float ShieldReplenishTime, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(
		ShieldBuffTimer,
		this,
		&UBuffComponent::ResetShield,
		BuffDuration
	);

	bIsReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ShieldReplenishTime;
	ShieldToReplenish += ShieldAmount;

	MulticastBuffShield(BuffDuration);
}

void UBuffComponent::DepleteShield()
{
	if (!bIsReplenishingShield)
	{
		GetWorld()->GetTimerManager().ClearTimer(ShieldBuffTimer);
		ResetShield();
	}
}

void UBuffComponent::BuffSpeed(const float SpeedMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeed,
		BuffDuration
	);

	MulticastBuffSpeed(SpeedMultiplier, BuffDuration);
}

void UBuffComponent::BuffJump(const float JumpMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJumpVelocity,
		BuffDuration
	);

	MulticastBuffJump(JumpMultiplier, BuffDuration);
}

void UBuffComponent::SetInitialJumpVelocity(const float JumpVelocity)
{
	InitialJumpVelocity = JumpVelocity;
}

void UBuffComponent::BuffDamage(const float DamageMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(
		DamageBuffTimer,
		this,
		&UBuffComponent::ResetDamage,
		BuffDuration
	);

	MulticastBuffDamage(DamageMultiplier, BuffDuration);
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	BuffDurationEntries = TArray<FBuffDurationEntry>();
}

void UBuffComponent::HealRampUp(const float DeltaTime)
{
	if (!bIsHealing || Character == nullptr || Character->IsEliminated())
	{
		return;
	}

	const float HealThisFrame = HealingRate * DeltaTime;
	const float HealthAmount = Character->GetHealth() + HealThisFrame;
	Character->SetHealth(FMath::Clamp(HealthAmount, 0.f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();

	AmountToHeal -= HealThisFrame;
	if (AmountToHeal <= 0.f || Character->IsFullHealth())
	{
		bIsHealing = false;
	}
}

void UBuffComponent::ShieldRampUp(const float DeltaTime)
{
	if (!bIsReplenishingShield || Character == nullptr || Character->IsEliminated())
	{
		return;
	}

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	const float ShieldAmount = Character->GetShield() + ReplenishThisFrame;
	Character->SetShield(FMath::Clamp(ShieldAmount, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();
	Character->UpdateHUDHealth();

	ShieldToReplenish -= ReplenishThisFrame;
	if (ShieldToReplenish <= 0.f)
	{
		bIsReplenishingShield = false;
	}
}

void UBuffComponent::UpdateBuffRemainingTimes(const float DeltaTime)
{
	if (!Character->IsLocallyControlled())
	{
		return;
	}

	if (ShieldBuffRemainingTime > 0.f)
	{
		ShieldBuffRemainingTime = FMath::Max(0.f, ShieldBuffRemainingTime - DeltaTime);
	}

	if (const int32* IndexPtr = BuffTypeToIndexMap.Find(EBuffType::EBT_Shield))
	{
		BuffDurationEntries[*IndexPtr].RemainingTime = ShieldBuffRemainingTime;
	}

	if (SpeedBuffRemainingTime > 0.f)
	{
		SpeedBuffRemainingTime = FMath::Max(0.f, SpeedBuffRemainingTime - DeltaTime);
	}

	if (const int32* IndexPtr = BuffTypeToIndexMap.Find(EBuffType::EBT_Speed))
	{
		BuffDurationEntries[*IndexPtr].RemainingTime = SpeedBuffRemainingTime;
	}

	if (JumpBuffRemainingTime > 0.f)
	{
		JumpBuffRemainingTime = FMath::Max(0.f, JumpBuffRemainingTime - DeltaTime);
	}

	if (const int32* IndexPtr = BuffTypeToIndexMap.Find(EBuffType::EBT_Jump))
	{
		BuffDurationEntries[*IndexPtr].RemainingTime = JumpBuffRemainingTime;
	}

	if (DamageBuffRemainingTime > 0.f)
	{
		DamageBuffRemainingTime = FMath::Max(0.f, DamageBuffRemainingTime - DeltaTime);
	}

	if (const int32* IndexPtr = BuffTypeToIndexMap.Find(EBuffType::EBT_Damage))
	{
		BuffDurationEntries[*IndexPtr].RemainingTime = DamageBuffRemainingTime;
	}
}

void UBuffComponent::SetBuffDurationEntries()
{
	if (!Character->IsLocallyControlled())
	{
		return;
	}

	BuffDurationEntries.Empty();
	BuffTypeToIndexMap.Empty();

	if (ShieldBuffRemainingTime > MIN_BUFF_REMAINING_TIME)
	{
		BuffDurationEntries.Add(FBuffDurationEntry(
			EBuffType::EBT_Shield, ShieldBuffDuration, ShieldBuffRemainingTime, ShieldBuffFillColor));
	}

	if (SpeedBuffRemainingTime > MIN_BUFF_REMAINING_TIME)
	{
		BuffDurationEntries.Add(FBuffDurationEntry(
			EBuffType::EBT_Speed, SpeedBuffDuration, SpeedBuffRemainingTime, SpeedBuffFillColor));
	}

	if (JumpBuffRemainingTime > MIN_BUFF_REMAINING_TIME)
	{
		BuffDurationEntries.Add(FBuffDurationEntry(
			EBuffType::EBT_Jump, JumpBuffDuration, JumpBuffRemainingTime, JumpBuffFillColor));
	}

	if (DamageBuffRemainingTime > MIN_BUFF_REMAINING_TIME)
	{
		BuffDurationEntries.Add(FBuffDurationEntry(
			EBuffType::EBT_Damage, DamageBuffDuration, DamageBuffRemainingTime, DamageBuffFillColor));
	}

	BuffDurationEntries.Sort([](const FBuffDurationEntry& A, const FBuffDurationEntry& B)
	{
		return A.RemainingTime > B.RemainingTime;
	});

	for (int32 Index = 0; Index < BuffDurationEntries.Num(); ++Index)
	{
		BuffTypeToIndexMap.Add(BuffDurationEntries[Index].BuffType, Index);
	}
}

void UBuffComponent::ResetShield()
{
	MulticastResetShield();
}

void UBuffComponent::MulticastBuffShield_Implementation(const float BuffDuration)
{
	HandleBuffShield(BuffDuration);
}

void UBuffComponent::MulticastResetShield_Implementation()
{
	HandleResetShield();
}

void UBuffComponent::HandleBuffShield(const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	ShieldBuffDuration = BuffDuration;
	ShieldBuffRemainingTime = BuffDuration;

	Character->SpawnShieldBuffEffect(ShieldBuffEffect);

	SetBuffDurationEntries();
}

void UBuffComponent::HandleResetShield()
{
	if (Character == nullptr)
	{
		return;
	}

	ShieldBuffRemainingTime = 0.f;

	Character->SetShield(0.f);
	Character->UpdateHUDShield();
	Character->UpdateHUDHealth();
	Character->SpawnShieldBuffEffect(nullptr);

	SetBuffDurationEntries();
}

void UBuffComponent::ResetSpeed()
{
	MulticastResetSpeed();
}

void UBuffComponent::MulticastBuffSpeed_Implementation(const float SpeedMultiplier, const float BuffDuration)
{
	HandleBuffSpeed(SpeedMultiplier, BuffDuration);
}

void UBuffComponent::MulticastResetSpeed_Implementation()
{
	HandleResetSpeed();
}

void UBuffComponent::HandleBuffSpeed(const float SpeedMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	SpeedBuffDuration = BuffDuration;
	SpeedBuffRemainingTime = BuffDuration;

	Character->SetAnimationPlayRate(SpeedMultiplier);

	if (const TObjectPtr<UCombatComponent> CombatComponent = Character->GetCombatComponent())
	{
		CombatComponent->SetSpeedMultiplier(SpeedMultiplier);
		CombatComponent->RecalculateMovementSpeed();
	}

	SetBuffDurationEntries();
}

void UBuffComponent::HandleResetSpeed()
{
	if (Character == nullptr)
	{
		return;
	}

	Character->SetAnimationPlayRate(1.f);

	if (const TObjectPtr<UCombatComponent> CombatComponent = Character->GetCombatComponent())
	{
		CombatComponent->SetSpeedMultiplier(1.f);
		CombatComponent->RecalculateMovementSpeed();
	}

	SetBuffDurationEntries();
}

void UBuffComponent::ResetJumpVelocity()
{
	MulticastResetJumpVelocity();
}

void UBuffComponent::MulticastBuffJump_Implementation(const float JumpMultiplier, const float BuffDuration)
{
	HandleBuffJump(JumpMultiplier, BuffDuration);
}

void UBuffComponent::MulticastResetJumpVelocity_Implementation()
{
	HandleResetJumpVelocity();
}

void UBuffComponent::HandleBuffJump(const float JumpMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	JumpBuffDuration = BuffDuration;
	JumpBuffRemainingTime = BuffDuration;

	Character->SetJumpSound(JumpSound);
	Character->SetJumpSoundAttenuation(JumpSoundAttenuation);

	if (const TObjectPtr<UCharacterMovementComponent> MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->JumpZVelocity = InitialJumpVelocity * JumpMultiplier;
	}

	SetBuffDurationEntries();
}

void UBuffComponent::HandleResetJumpVelocity()
{
	if (Character == nullptr)
	{
		return;
	}

	Character->SetJumpSound(nullptr);
	Character->SetJumpSoundAttenuation(nullptr);

	if (const TObjectPtr<UCharacterMovementComponent> MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->JumpZVelocity = InitialJumpVelocity;
	}

	SetBuffDurationEntries();
}

void UBuffComponent::ResetDamage()
{
	MulticastResetDamage();
}

void UBuffComponent::MulticastBuffDamage_Implementation(const float DamageMultiplier, const float BuffDuration)
{
	HandleBuffDamage(DamageMultiplier, BuffDuration);
}

void UBuffComponent::MulticastResetDamage_Implementation()
{
	HandleResetDamage();
}

void UBuffComponent::HandleBuffDamage(const float DamageMultiplier, const float BuffDuration)
{
	if (Character == nullptr)
	{
		return;
	}

	DamageBuffDuration = BuffDuration;
	DamageBuffRemainingTime = BuffDuration;

	Character->SetDamageBuffEffect(DamageBuffEffect);

	if (const TObjectPtr<AWeapon> EquippedWeapon = Character->GetEquippedWeapon())
	{
		EquippedWeapon->SpawnDamageBuffEffect(DamageBuffEffect);
	}

	if (const TObjectPtr<UCombatComponent> CombatComponent = Character->GetCombatComponent())
	{
		CombatComponent->SetDamageMultiplier(DamageMultiplier);
	}

	SetBuffDurationEntries();
}

void UBuffComponent::HandleResetDamage()
{
	if (Character == nullptr)
	{
		return;
	}

	Character->SetDamageBuffEffect(nullptr);

	if (const TObjectPtr<AWeapon> EquippedWeapon = Character->GetEquippedWeapon())
	{
		EquippedWeapon->SpawnDamageBuffEffect(nullptr);
	}

	if (const TObjectPtr<UCombatComponent> CombatComponent = Character->GetCombatComponent())
	{
		CombatComponent->SetDamageMultiplier(1.f);
	}

	SetBuffDurationEntries();
}
