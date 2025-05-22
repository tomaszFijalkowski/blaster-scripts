// Fill out your copyright notice in the Description page of Project Settings.

#include "LagCompensationComponent.h"
#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/Weapon/BodyshotDamageType.h"
#include "Blaster/Weapon/HeadshotDamageType.h"
#include "Blaster/Weapon/HitscanWeapon.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFrameHistory();
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& FramePackage, const FColor Color) const
{
	for (auto& HitboxInfo : FramePackage.HitboxInfoMap)
	{
		DrawDebugBox(
			GetWorld(),
			HitboxInfo.Value.Location,
			HitboxInfo.Value.BoxExtent,
			FQuat(HitboxInfo.Value.Rotation),
			Color,
			false,
			MaxRecordTime
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter,
                                                                    const FVector_NetQuantize& TraceStart,
                                                                    const FVector_NetQuantize& HitLocation,
                                                                    const float HitTime)
{
	const FFramePackage FramePackageToCheck = GetFramePackageToCheck(HitCharacter, HitTime);
	return ConfirmHit(FramePackageToCheck, TraceStart, HitLocation);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, const float HitTime)
{
	TArray<FFramePackage> FramePackagesToCheck;

	for (const auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr)
		{
			continue;
		}

		FramePackagesToCheck.Add(GetFramePackageToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramePackagesToCheck, TraceStart, HitLocations);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(
	ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize100& InitialVelocity, const float HitTime)
{
	const FFramePackage FramePackageToCheck = GetFramePackageToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FramePackageToCheck, TraceStart, InitialVelocity, HitTime);
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart,
                                                                  const FVector_NetQuantize& HitLocation,
                                                                  const float HitTime, AWeapon* DamageCauser)
{
	const FServerSideRewindResult Result = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	if (HitCharacter && DamageCauser && Character && Result.bHitConfirmed)
	{
		const float Distance = FVector::Dist(TraceStart, HitCharacter->GetActorLocation());
		const float BaseDamage = Result.bIsHeadshot ? DamageCauser->GetHeadshotDamage() : DamageCauser->GetDamage();
		const float DamageToApply = DamageCauser->CalculateDamageWithFalloff(BaseDamage, Distance);

		const TSubclassOf<UDamageType> DamageTypeClass = Result.bIsHeadshot
			                                                 ? UHeadshotDamageType::StaticClass()
			                                                 : UBodyshotDamageType::StaticClass();
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToApply,
			Character->GetController(),
			DamageCauser,
			DamageTypeClass
		);
	}
}

void ULagCompensationComponent::ServerShotgunScoreRequest_Implementation(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, const float HitTime, AWeapon* DamageCauser)
{
	const FShotgunServerSideRewindResult Result = ShotgunServerSideRewind(
		HitCharacters, TraceStart, HitLocations, HitTime);

	for (const auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character == nullptr || DamageCauser == nullptr)
		{
			continue;
		}

		float TotalDamage = 0.f;
		bool bIsHeadshot = false;

		if (Result.Headshots.Contains(HitCharacter))
		{
			const float HeadshotDamage = Result.Headshots[HitCharacter] * DamageCauser->GetHeadshotDamage();
			TotalDamage += HeadshotDamage;
			bIsHeadshot = true;
		}

		if (Result.Bodyshots.Contains(HitCharacter))
		{
			const float BodyshotDamage = Result.Bodyshots[HitCharacter] * DamageCauser->GetDamage();
			TotalDamage += BodyshotDamage;
		}

		const float Distance = FVector::Dist(TraceStart, HitCharacter->GetActorLocation());
		const float DamageToApply = DamageCauser->CalculateDamageWithFalloff(TotalDamage, Distance);

		const TSubclassOf<UDamageType> DamageTypeClass = bIsHeadshot
			                                                 ? UHeadshotDamageType::StaticClass()
			                                                 : UBodyshotDamageType::StaticClass();
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToApply,
			Character->GetController(),
			DamageCauser,
			DamageTypeClass
		);
	}
}

void ULagCompensationComponent::ServerProjectileScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
                                                                            const FVector_NetQuantize& TraceStart,
                                                                            const FVector_NetQuantize100&
                                                                            InitialVelocity, const float HitTime,
                                                                            AWeapon* DamageCauser)
{
	const FServerSideRewindResult Result = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity,
	                                                                  HitTime);
	if (HitCharacter && DamageCauser && Character && Result.bHitConfirmed)
	{
		const float Distance = FVector::Dist(TraceStart, HitCharacter->GetActorLocation());
		const float BaseDamage = Result.bIsHeadshot ? DamageCauser->GetHeadshotDamage() : DamageCauser->GetDamage();
		const float DamageToApply = DamageCauser->CalculateDamageWithFalloff(BaseDamage, Distance);

		const TSubclassOf<UDamageType> DamageTypeClass = Result.bIsHeadshot
			                                                 ? UHeadshotDamageType::StaticClass()
			                                                 : UBodyshotDamageType::StaticClass();
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToApply,
			Character->GetController(),
			DamageCauser,
			DamageTypeClass
		);
	}
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage FramePackage;
	SaveFramePackage(FramePackage);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& FramePackage)
{
	FramePackage.Time = GetWorld()->GetTimeSeconds();

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character.Get();
	if (Character)
	{
		FramePackage.Character = Character;

		for (const auto& HitboxPair : Character->GetHitboxes())
		{
			FHitboxInformation HitboxInfo;
			HitboxInfo.Location = HitboxPair.Value->GetComponentLocation();
			HitboxInfo.Rotation = HitboxPair.Value->GetComponentRotation();
			HitboxInfo.BoxExtent = HitboxPair.Value->GetScaledBoxExtent();

			FramePackage.HitboxInfoMap.Add(HitboxPair.Key, HitboxInfo);
		}
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFramePackages(const FFramePackage& YoungerFramePackage,
                                                                    const FFramePackage& OlderFramePackage,
                                                                    const float HitTime)
{
	const float Distance = YoungerFramePackage.Time - OlderFramePackage.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFramePackage.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (const auto& YoungerFramePackagePair : YoungerFramePackage.HitboxInfoMap)
	{
		const FName& HitboxInfoName = YoungerFramePackagePair.Key;
		const FHitboxInformation& YoungerHitbox = YoungerFramePackagePair.Value;
		const FHitboxInformation& OlderHitbox = OlderFramePackage.HitboxInfoMap[HitboxInfoName];

		FHitboxInformation InterpHitboxInfo;
		InterpHitboxInfo.Location = FMath::VInterpTo(OlderHitbox.Location, YoungerHitbox.Location, 1.f, InterpFraction);
		InterpHitboxInfo.Rotation = FMath::RInterpTo(OlderHitbox.Rotation, YoungerHitbox.Rotation, 1.f, InterpFraction);
		InterpHitboxInfo.BoxExtent = YoungerHitbox.BoxExtent;

		InterpFramePackage.HitboxInfoMap.Add(HitboxInfoName, InterpHitboxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& FramePackage,
                                                              const FVector_NetQuantize& TraceStart,
                                                              const FVector_NetQuantize& HitLocation)
{
	const ABlasterCharacter* HitCharacter = FramePackage.Character ? FramePackage.Character.Get() : nullptr;
	if (HitCharacter == nullptr)
	{
		return FServerSideRewindResult();
	}

	FFramePackage CurrentFramePackage;
	CacheHitboxPositions(HitCharacter, CurrentFramePackage);
	MoveHitboxes(HitCharacter, FramePackage);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadHitbox = HitCharacter->GetHitboxes()[Bone::HeadBone];
	HeadHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadHitbox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	if (const UWorld* World = GetWorld())
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * LINE_TRACE_MULTIPLIER;

		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_Hitbox
		);

		if (ConfirmHitResult.bBlockingHit) // Hit the head, return early
		{
			// if (ConfirmHitResult.Component.IsValid())
			// {
			// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
			// 	{
			// 		DrawDebugBox(
			// 			GetWorld(),
			// 			Hitbox->GetComponentLocation(),
			// 			Hitbox->GetScaledBoxExtent(),
			// 			FQuat(Hitbox->GetComponentRotation()),
			// 			FColor::Red,
			// 			false,
			// 			8.f
			// 		);
			// 	}
			// }

			MoveHitboxes(HitCharacter, CurrentFramePackage, true);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{true, true};
		}

		// Didn't hit the head, check the rest of the hitboxes
		for (const auto& HitboxPair : HitCharacter->GetHitboxes())
		{
			if (HitboxPair.Value != nullptr)
			{
				HitboxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitboxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
			}
		}

		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_Hitbox
		);

		if (ConfirmHitResult.bBlockingHit)
		{
			// if (ConfirmHitResult.Component.IsValid())
			// {
			// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
			// 	{
			// 		DrawDebugBox(
			// 			GetWorld(),
			// 			Hitbox->GetComponentLocation(),
			// 			Hitbox->GetScaledBoxExtent(),
			// 			FQuat(Hitbox->GetComponentRotation()),
			// 			FColor::Blue,
			// 			false,
			// 			8.f
			// 		);
			// 	}
			// }

			MoveHitboxes(HitCharacter, CurrentFramePackage, true);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{true, false};
		}
	}

	// Reset hitboxes
	MoveHitboxes(HitCharacter, CurrentFramePackage, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{false, false};
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
                                                                            const FVector_NetQuantize& TraceStart,
                                                                            const TArray<FVector_NetQuantize>&
                                                                            HitLocations)
{
	TArray<FFramePackage> CurrentFramePackages;
	FShotgunServerSideRewindResult ShotgunResult;

	for (const auto& FramePackage : FramePackages)
	{
		ABlasterCharacter* HitCharacter = FramePackage.Character ? FramePackage.Character.Get() : nullptr;
		if (HitCharacter == nullptr)
		{
			continue;
		}

		FFramePackage CurrentFramePackage;
		CurrentFramePackage.Character = HitCharacter;

		CacheHitboxPositions(HitCharacter, CurrentFramePackage);
		MoveHitboxes(HitCharacter, FramePackage);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

		CurrentFramePackages.Add(CurrentFramePackage);

		// Enable collision for the head first
		UBoxComponent* HeadHitbox = HitCharacter->GetHitboxes()[Bone::HeadBone];
		HeadHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HeadHitbox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
	}

	// Check for headshots
	if (const UWorld* World = GetWorld())
	{
		for (const auto& HitLocation : HitLocations)
		{
			FHitResult ConfirmHitResult;
			const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * LINE_TRACE_MULTIPLIER;

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_Hitbox
			);

			if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
			{
				if (ShotgunResult.Headshots.Contains(HitCharacter))
				{
					ShotgunResult.Headshots[HitCharacter]++;
				}
				else
				{
					ShotgunResult.Headshots.Emplace(HitCharacter, 1);
				}

				// if (ConfirmHitResult.Component.IsValid())
				// {
				// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
				// 	{
				// 		DrawDebugBox(
				// 			GetWorld(),
				// 			Hitbox->GetComponentLocation(),
				// 			Hitbox->GetScaledBoxExtent(),
				// 			FQuat(Hitbox->GetComponentRotation()),
				// 			FColor::Red,
				// 			false,
				// 			8.f
				// 		);
				// 	}
				// }
			}
		}
	}

	// Enable collision for all hitboxes, then disable head hitbox
	for (const auto& FramePackage : FramePackages)
	{
		const ABlasterCharacter* HitCharacter = FramePackage.Character ? FramePackage.Character.Get() : nullptr;
		if (HitCharacter == nullptr)
		{
			continue;
		}

		for (const auto& HitboxPair : HitCharacter->GetHitboxes())
		{
			if (HitboxPair.Value != nullptr)
			{
				HitboxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitboxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
			}
		}

		UBoxComponent* HeadHitbox = HitCharacter->GetHitboxes()[Bone::HeadBone];
		HeadHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Check for bodyshots
	if (const UWorld* World = GetWorld())
	{
		for (const auto& HitLocation : HitLocations)
		{
			FHitResult ConfirmHitResult;
			const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * LINE_TRACE_MULTIPLIER;

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_Hitbox
			);

			if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
			{
				// TODO fix double damage when pellet hits both head and body
				if (ShotgunResult.Bodyshots.Contains(HitCharacter))
				{
					ShotgunResult.Bodyshots[HitCharacter]++;
				}
				else
				{
					ShotgunResult.Bodyshots.Emplace(HitCharacter, 1);
				}

				// if (ConfirmHitResult.Component.IsValid())
				// {
				// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
				// 	{
				// 		DrawDebugBox(
				// 			GetWorld(),
				// 			Hitbox->GetComponentLocation(),
				// 			Hitbox->GetScaledBoxExtent(),
				// 			FQuat(Hitbox->GetComponentRotation()),
				// 			FColor::Blue,
				// 			false,
				// 			8.f
				// 		);
				// 	}
				// }
			}
		}
	}

	// Reset hitboxes
	for (const auto& CurrentFramePackage : CurrentFramePackages)
	{
		const ABlasterCharacter* HitCharacter = CurrentFramePackage.Character
			                                        ? CurrentFramePackage.Character.Get()
			                                        : nullptr;
		if (HitCharacter == nullptr)
		{
			continue;
		}

		MoveHitboxes(HitCharacter, CurrentFramePackage, true);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& FramePackage,
                                                                        const FVector_NetQuantize& TraceStart,
                                                                        const FVector_NetQuantize100& InitialVelocity,
                                                                        float HitTime)
{
	const ABlasterCharacter* HitCharacter = FramePackage.Character ? FramePackage.Character.Get() : nullptr;
	if (HitCharacter == nullptr)
	{
		return FServerSideRewindResult();
	}

	FFramePackage CurrentFramePackage;
	CacheHitboxPositions(HitCharacter, CurrentFramePackage);
	MoveHitboxes(HitCharacter, FramePackage);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadHitbox = HitCharacter->GetHitboxes()[Bone::HeadBone];
	HeadHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadHitbox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.MaxSimTime = MAX_SIM_TIME;
	PathParams.SimFrequency = SIM_FREQUENCY;
	PathParams.ProjectileRadius = 4.f;
	PathParams.StartLocation = TraceStart;
	PathParams.TraceChannel = ECC_Hitbox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	FHitResult ConfirmHitResult = PathResult.HitResult;
	if (ConfirmHitResult.bBlockingHit) // Hit the head, return early
	{
		// if (ConfirmHitResult.Component.IsValid())
		// {
		// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
		// 	{
		// 		DrawDebugBox(
		// 			GetWorld(),
		// 			Hitbox->GetComponentLocation(),
		// 			Hitbox->GetScaledBoxExtent(),
		// 			FQuat(Hitbox->GetComponentRotation()),
		// 			FColor::Red,
		// 			false,
		// 			8.f
		// 		);
		// 	}
		// }

		MoveHitboxes(HitCharacter, CurrentFramePackage, true);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		return FServerSideRewindResult{true, true};
	}

	// Didn't hit the head, check the rest of the hitboxes
	for (const auto& HitboxPair : HitCharacter->GetHitboxes())
	{
		if (HitboxPair.Value != nullptr)
		{
			HitboxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			HitboxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
		}
	}

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	ConfirmHitResult = PathResult.HitResult;
	if (ConfirmHitResult.bBlockingHit)
	{
		// if (ConfirmHitResult.Component.IsValid())
		// {
		// 	if (const UBoxComponent* Hitbox = Cast<UBoxComponent>(ConfirmHitResult.Component.Get()))
		// 	{
		// 		DrawDebugBox(
		// 			GetWorld(),
		// 			Hitbox->GetComponentLocation(),
		// 			Hitbox->GetScaledBoxExtent(),
		// 			FQuat(Hitbox->GetComponentRotation()),
		// 			FColor::Blue,
		// 			false,
		// 			8.f
		// 		);
		// 	}
		// }

		MoveHitboxes(HitCharacter, CurrentFramePackage, true);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		return FServerSideRewindResult{true, false};
	}

	// Reset hitboxes
	MoveHitboxes(HitCharacter, CurrentFramePackage, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	return FServerSideRewindResult{false, false};
}

void ULagCompensationComponent::CacheHitboxPositions(const ABlasterCharacter* HitCharacter,
                                                     FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	for (const auto& HitboxPair : HitCharacter->GetHitboxes())
	{
		if (HitboxPair.Value != nullptr)
		{
			FHitboxInformation HitboxInfo;
			HitboxInfo.Location = HitboxPair.Value->GetComponentLocation();
			HitboxInfo.Rotation = HitboxPair.Value->GetComponentRotation();
			HitboxInfo.BoxExtent = HitboxPair.Value->GetScaledBoxExtent();

			OutFramePackage.HitboxInfoMap.Add(HitboxPair.Key, HitboxInfo);
		}
	}
}

void ULagCompensationComponent::MoveHitboxes(const ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage,
                                             const bool bResetCollision)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	for (const auto& HitboxPair : HitCharacter->GetHitboxes())
	{
		if (HitboxPair.Value != nullptr)
		{
			HitboxPair.Value->SetWorldLocation(FramePackage.HitboxInfoMap[HitboxPair.Key].Location);
			HitboxPair.Value->SetWorldRotation(FramePackage.HitboxInfoMap[HitboxPair.Key].Rotation);
			HitboxPair.Value->SetBoxExtent(FramePackage.HitboxInfoMap[HitboxPair.Key].BoxExtent);

			if (bResetCollision)
			{
				HitboxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(const ABlasterCharacter* InCharacter,
                                                             const ECollisionEnabled::Type CollisionEnabled) const
{
	if (InCharacter == nullptr)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = InCharacter->GetMesh())
	{
		Mesh->SetCollisionEnabled(CollisionEnabled);
	}
}

FFramePackage ULagCompensationComponent::GetFramePackageToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	if (HitCharacter == nullptr || HitCharacter->GetLagCompensationComponent() == nullptr)
	{
		return FFramePackage();
	}

	const TRingBuffer<FFramePackage>& HitFrameHistory = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	if (HitFrameHistory.Num() < 2)
	{
		return FFramePackage();
	}

	FFramePackage FramePackageToCheck;
	bool bShouldInterpolate = true;

	const float YoungestHistoryTime = HitFrameHistory.Last().Time;
	const float OldestHistoryTime = HitFrameHistory.First().Time;

	if (OldestHistoryTime >= HitTime)
	{
		FramePackageToCheck = HitFrameHistory.First();
		bShouldInterpolate = false;
	}

	if (YoungestHistoryTime <= HitTime)
	{
		FramePackageToCheck = HitFrameHistory.Last();
		bShouldInterpolate = false;
	}

	FFramePackage YoungerFramePackage = HitFrameHistory.Last();
	FFramePackage OlderFramePackage = YoungerFramePackage;

	for (int32 Index = HitFrameHistory.Num() - 1; Index >= 0; --Index)
	{
		OlderFramePackage = HitFrameHistory[Index];
		if (OlderFramePackage.Time <= HitTime)
		{
			break;
		}
		YoungerFramePackage = OlderFramePackage;
	}

	if (OlderFramePackage.Time == HitTime)
	{
		FramePackageToCheck = OlderFramePackage;
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		FramePackageToCheck = InterpBetweenFramePackages(YoungerFramePackage, OlderFramePackage, HitTime);
	}

	FramePackageToCheck.Character = HitCharacter;
	return FramePackageToCheck;
}

void ULagCompensationComponent::SaveFrameHistory()
{
	if (Character == nullptr || !Character->HasAuthority())
	{
		return;
	}

	FFramePackage CurrentFramePackage;
	SaveFramePackage(CurrentFramePackage);

	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddFront(CurrentFramePackage);
	}
	else
	{
		float HistoryLength = CalculateFrameHistoryLength();
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.PopFront();
			HistoryLength = CalculateFrameHistoryLength();
		}

		FrameHistory.Add(CurrentFramePackage);

		// ShowFramePackage(CurrentFramePackage, FColor::Red);
	}
}

float ULagCompensationComponent::CalculateFrameHistoryLength() const
{
	if (FrameHistory.Num() <= 1)
	{
		return 0.f;
	}

	const float HeadTime = FrameHistory.Last().Time;
	const float TailTime = FrameHistory.First().Time;
	return HeadTime - TailTime;
}
