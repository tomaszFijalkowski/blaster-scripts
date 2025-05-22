// Fill out your copyright notice in the Description page of Project Settings.

#include "Shotgun.h"
#include "BodyshotDamageType.h"
#include "HeadshotDamageType.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}

	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(Socket::MuzzleFlash))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector TraceStart = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> BodyshotMap;
		TMap<ABlasterCharacter*, uint32> HeadshotMap;

		FireHits.Empty();
		ProcessedImpactPoints.Empty();

		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(TraceStart, HitTarget, FireHit);

			const bool bIsHeadshot = FireHit.BoneName.ToString() == Bone::HeadBone;

			if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
			{
				if (bIsHeadshot)
				{
					if (HeadshotMap.Contains(HitCharacter))
					{
						HeadshotMap[HitCharacter]++;
					}
					else
					{
						HeadshotMap.Emplace(HitCharacter, 1);
					}
				}
				else
				{
					if (BodyshotMap.Contains(HitCharacter))
					{
						BodyshotMap[HitCharacter]++;
					}
					else
					{
						BodyshotMap.Emplace(HitCharacter, 1);
					}
				}
			}

			FireHits.Add({FireHit, bIsHeadshot});
		}

		FireHits.Sort([](const FFireHit& A, const FFireHit& B)
		{
			return A.bIsHeadshot > B.bIsHeadshot;
		});

		for (const auto& [FireHit, bIsHeadshot] : FireHits)
		{
			if (const UPhysicalMaterial* PhysMaterial = FireHit.PhysMaterial.Get())
			{
				SetImpactEffects(PhysMaterial->SurfaceType, bIsHeadshot);
			}
			else
			{
				SetImpactEffects(SurfaceType_Default, bIsHeadshot);
			}

			PlayImpactEffects(FireHit);

			ProcessedImpactPoints.Add(FireHit.Location);
		}

		TArray<ABlasterCharacter*> HitCharacters;
		TMap<ABlasterCharacter*, float> DamageToApplyMap;

		// Calculate bodyshot damage by multiplying the number of pellets by the damage and store the hit characters
		for (const auto BodyshotHitPair : BodyshotMap)
		{
			if (BodyshotHitPair.Key)
			{
				DamageToApplyMap.Emplace(BodyshotHitPair.Key, BodyshotHitPair.Value * Damage);
				HitCharacters.AddUnique(BodyshotHitPair.Key);
			}
		}

		// Calculate headshot damage by multiplying the number of pellets by the headshot damage and store the hit characters
		for (const auto HeadshotHitPair : HeadshotMap)
		{
			if (HeadshotHitPair.Key)
			{
				if (DamageToApplyMap.Contains(HeadshotHitPair.Key))
				{
					DamageToApplyMap[HeadshotHitPair.Key] += HeadshotHitPair.Value * HeadshotDamage;
				}
				else
				{
					DamageToApplyMap.Emplace(HeadshotHitPair.Key, HeadshotHitPair.Value * HeadshotDamage);
				}

				HitCharacters.AddUnique(HeadshotHitPair.Key);
			}
		}

		AController* InstigatorController = OwnerPawn->GetController();

		// Apply total damage to hit characters
		for (const auto DamageToApplyPair : DamageToApplyMap)
		{
			ABlasterCharacter* HitCharacter = DamageToApplyPair.Key;
			if (HitCharacter && InstigatorController)
			{
				const bool bCauseAuthorityDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthorityDamage)
				{
					const float Distance = FVector::Dist(TraceStart, HitCharacter->GetActorLocation());
					const float DamageToApply = CalculateDamageWithFalloff(DamageToApplyPair.Value, Distance);

					const bool bIsHeadshot = HeadshotMap.Contains(HitCharacter);
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
			}
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
				OwnerCharacter->GetLagCompensationComponent()->ServerShotgunScoreRequest(
					HitCharacters,
					TraceStart,
					HitTargets,
					HitTime,
					this
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutTraceEnds,
                                          const bool bIsAiming, const float AimAccuracyFactor) const
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(Socket::MuzzleFlash);
	if (MuzzleFlashSocket == nullptr)
	{
		return;
	}

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

	const FVector TraceStart = SocketTransform.GetLocation();
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();

	// Blend between regular and aiming scatter based on aim accuracy factor
	float SphereRadius;
	if (bIsAiming)
	{
		SphereRadius = FMath::Lerp(ScatterSphereRadius, AimingScatterSphereRadius, AimAccuracyFactor);
	}
	else
	{
		SphereRadius = ScatterSphereRadius;
	}

	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToScatterSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLocation = SphereCenter + RandVector;
		const FVector ToEndLocation = EndLocation - TraceStart;
		const FVector TraceEnd = FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());

		OutTraceEnds.Add(TraceEnd);
	}
}

void AShotgun::SetImpactEffects(const EPhysicalSurface PhysicalSurface, const bool bIsHeadshot)
{
	Super::SetImpactEffects(PhysicalSurface, bIsHeadshot);
}

void AShotgun::PlayImpactEffects(const FHitResult& FireHit, const bool bPlaySound) const
{
	int32 NearbyImpactsCount = 0;
	for (const auto& ImpactPoint : ProcessedImpactPoints)
	{
		const float Distance = FVector::Dist(FireHit.ImpactPoint, ImpactPoint);
		if (Distance < MinImpactDistance)
		{
			NearbyImpactsCount++;
			if (NearbyImpactsCount >= MaxNearbyImpacts)
			{
				break;
			}
		}
	}

	if (NearbyImpactsCount < MaxNearbyImpacts)
	{
		Super::PlayImpactEffects(FireHit, true);
	}
	else
	{
		Super::PlayImpactEffects(FireHit, false);
	}
}
