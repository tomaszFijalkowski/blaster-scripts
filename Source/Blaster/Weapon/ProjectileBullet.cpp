// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBullet.h"
#include "BodyshotDamageType.h"
#include "HeadshotDamageType.h"
#include "Weapon.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	const FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              const FVector NormalImpulse, const FHitResult& Hit)
{
	if (const ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner()))
	{
		if (ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->GetController()))
		{
			AWeapon* OwnerWeapon = OwnerCharacter->GetEquippedWeapon();
			if (OwnerWeapon && OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				const bool bIsHeadshot = Hit.BoneName.ToString() == Bone::HeadBone;
				const FVector ProjectileStartLocation = OwnerWeapon->GetMuzzleFlashSocketLocation();
				const float Distance = FVector::Dist(ProjectileStartLocation, OtherActor->GetActorLocation());
				const float BaseDamage = bIsHeadshot ? HeadshotDamage : Damage;
				const float DamageToApply = OwnerWeapon->CalculateDamageWithFalloff(BaseDamage, Distance);

				const TSubclassOf<UDamageType> DamageTypeClass = bIsHeadshot
					                                                 ? UHeadshotDamageType::StaticClass()
					                                                 : UBodyshotDamageType::StaticClass();
				UGameplayStatics::ApplyDamage(
					OtherActor,
					DamageToApply,
					OwnerController,
					this,
					DamageTypeClass
				);

				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}

			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if (bUseServerSideRewind && HitCharacter &&
				OwnerCharacter->IsLocallyControlled() &&
				OwnerCharacter->GetLagCompensationComponent())
			{
				const float HitTime = OwnerController->GetServerTime() - OwnerController->GetSingleTripTime();
				OwnerCharacter->GetLagCompensationComponent()->ServerProjectileScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					HitTime,
					OwnerWeapon
				);
			}
		}

		Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
	}
}
