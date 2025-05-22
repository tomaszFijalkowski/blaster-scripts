// Fill out your copyright notice in the Description page of Project Settings.

#include "HitscanWeapon.h"
#include "BodyshotDamageType.h"
#include "HeadshotDamageType.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void AHitscanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}

	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(Socket::MuzzleFlash))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector TraceStart = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(TraceStart, HitTarget, FireHit);

		if (FireHit.bBlockingHit)
		{
			const bool bIsHeadshot = FireHit.BoneName.ToString() == Bone::HeadBone;

			AController* InstigatorController = OwnerPawn->GetController();
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (InstigatorController && HitCharacter)
			{
				const bool bCauseAuthorityDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthorityDamage)
				{
					const float Distance = FVector::Dist(TraceStart, HitCharacter->GetActorLocation());
					const float BaseDamage = bIsHeadshot ? HeadshotDamage : Damage;
					const float DamageToApply = CalculateDamageWithFalloff(BaseDamage, Distance);

					const TSubclassOf<UDamageType> DamageTypeClass = bIsHeadshot
						                                                 ? UHeadshotDamageType::StaticClass()
						                                                 : UBodyshotDamageType::StaticClass();
					UGameplayStatics::ApplyDamage(
						HitCharacter,
						DamageToApply,
						InstigatorController,
						this,
						DamageTypeClass
					);
				}

				if (!HasAuthority() && bUseServerSideRewind)
				{
					OwnerCharacter = OwnerCharacter == nullptr
						                 ? Cast<ABlasterCharacter>(Owner)
						                 : OwnerCharacter.Get();
					OwnerController = OwnerController == nullptr
						                  ? Cast<ABlasterPlayerController>(InstigatorController)
						                  : OwnerController.Get();

					if (OwnerCharacter && OwnerCharacter->GetLagCompensationComponent() &&
						OwnerCharacter->IsLocallyControlled() && OwnerController)
					{
						const float HitTime = OwnerController->GetServerTime() - OwnerController->GetSingleTripTime();
						OwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest(
							HitCharacter,
							TraceStart,
							HitTarget,
							HitTime,
							this
						);
					}
				}
			}

			if (const UPhysicalMaterial* PhysMaterial = FireHit.PhysMaterial.Get())
			{
				SetImpactEffects(PhysMaterial->SurfaceType, bIsHeadshot);
			}
			else
			{
				SetImpactEffects(SurfaceType_Default, bIsHeadshot);
			}

			PlayImpactEffects(FireHit);
		}
	}
}

void AHitscanWeapon::SetImpactEffects(const EPhysicalSurface PhysicalSurface, const bool bIsHeadshot)
{
	switch (PhysicalSurface)
	{
	case EPS_Player:
		ImpactParticlesToSpawn = PlayerImpactParticles;
		ImpactSoundToPlay = bIsHeadshot ? HeadshotImpactSound : BodyshotImpactSound;
		break;
	case EPS_Metal:
		ImpactParticlesToSpawn = MetalImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	case EPS_Stone:
		ImpactParticlesToSpawn = StoneImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	case EPS_Wood:
		ImpactParticlesToSpawn = WoodImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	default:
		ImpactParticlesToSpawn = StoneImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
	}
}

void AHitscanWeapon::PlayImpactEffects(const FHitResult& FireHit, const bool bPlaySound) const
{
	if (ImpactParticlesToSpawn)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			this,
			ImpactParticlesToSpawn,
			FireHit.ImpactPoint,
			FireHit.ImpactNormal.Rotation()
		);
	}

	if (ImpactSoundToPlay && bPlaySound)
	{
		const APawn* OwnerPawn = Cast<APawn>(GetOwner());
		const bool bIsLocallyControlled = OwnerPawn && OwnerPawn->IsLocallyControlled();
		const bool bHitPlayer = ImpactSoundToPlay == HeadshotImpactSound || ImpactSoundToPlay == BodyshotImpactSound;

		if (bIsLocallyControlled && bHitPlayer)
		{
			UGameplayStatics::PlaySound2D(this, ImpactSoundToPlay, SOUND_2D_VOLUME_MULTIPLIER);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSoundToPlay, FireHit.ImpactPoint);
		}
	}
}

void AHitscanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit) const
{
	if (const UWorld* World = GetWorld())
	{
		const FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * LINE_TRACE_MULTIPLIER;

		FCollisionQueryParams Params;
		Params.bReturnPhysicalMaterial = true;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			Params
		);

		FVector BeamEnd = TraceEnd;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}

		// DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Red, true, 1.f);

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);

			if (Beam)
			{
				Beam->SetVectorParameter(ParticleSystemParameter::Target, BeamEnd);
			}
		}
	}
}
