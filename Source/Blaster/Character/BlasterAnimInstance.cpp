// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr)
	{
		Character = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (Character == nullptr)
	{
		return;
	}

	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsFalling = Character->GetCharacterMovement()->IsFalling();
	bIsAccelerating = Character->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bWeaponEquipped = Character->IsWeaponEquipped();
	EquippedWeapon = Character->GetEquippedWeapon();
	bIsCrouched = Character->bIsCrouched;
	bIsAiming = Character->IsAiming();
	bRotateRootBone = Character->GetRotateRootBone();
	TurningInPlace = Character->GetTurningInPlace();
	bIsEliminated = Character->IsEliminated();
	PlayRate = Character->GetAnimationPlayRate();
	FABRIKBlendTime = Character->GetFABRIKBlendTime();

	// Offset Yaw for Strafing
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(Character->GetVelocity());
	const FRotator AimRotation = Character->GetBaseAimRotation();
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

	CalculateLean(DeltaSeconds);

	AO_Yaw = Character->GetAO_Yaw();
	AO_Pitch = Character->GetAO_Pitch();

	CalculateHandPositions(DeltaSeconds);
}

void UBlasterAnimInstance::CalculateLean(const float DeltaSeconds)
{
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = Character->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, LEAN_INTERP_SPEED);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
}

void UBlasterAnimInstance::CalculateHandPositions(const float DeltaSeconds)
{
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && Character->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(Socket::LeftHandSocket, RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		Character->GetMesh()->TransformToBoneSpace(Bone::RightHandBone, LeftHandTransform.GetLocation(),
		                                           FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (Character->IsLocallyControlled())
		{
			bIsLocallyControlled = true;

			const FTransform RightHandTransform = Character->GetMesh()->GetSocketTransform(
				Bone::RightHandBone, RTS_World);

			const FVector RightHandLocation = RightHandTransform.GetLocation();
			const FVector DirectionToTarget = RightHandLocation - Character->GetHitTarget();
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandLocation, RightHandLocation + DirectionToTarget);

			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds,
			                                     HAND_ROTATION_INTERP_SPEED);

			// Smooth out rotation for rapid pitch changes
			const FRotator RotationDifference = (LookAtRotation - RightHandRotation).GetNormalized();
			if (FMath::Abs(RotationDifference.Pitch) > 2.f)
			{
				RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds,
				                                     HAND_ROTATION_INTERP_SPEED);
			}
			else
			{
				RightHandRotation = LookAtRotation;
			}
		}
	}

	bUseFABRIK = Character->IsUsingFABRIK();
	bUseAimOffsets = Character->GetCombatState() == ECombatState::ECS_Unoccupied && !Character->IsLookAroundOnly();
	bTransformRightHand = Character->GetCombatState() == ECombatState::ECS_Unoccupied && !Character->IsLookAroundOnly();
	bIsHoldingFlag = Character->IsHoldingFlag();
}
