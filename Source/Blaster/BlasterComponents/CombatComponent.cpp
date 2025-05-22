// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"
#include "TimerManager.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/Weapon/Shotgun.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshair(DeltaTime);
		InterpFOV(DeltaTime);
		SetSniperScope();
		SetReloadProgress();
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, Flag);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, ThrowingGrenades);
	DOREPLIFETIME(UCombatComponent, bIsHoldingFlag);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	const bool bCannotEquipWeapon = Character == nullptr || CombatState != ECombatState::ECS_Unoccupied ||
		WeaponToEquip == nullptr || WeaponToEquip->IsFalling() || bIsHoldingFlag;
	if (bCannotEquipWeapon)
	{
		return;
	}

	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		if (AFlag* FlagToEquip = Cast<AFlag>(WeaponToEquip))
		{
			EquipFlag(Cast<AFlag>(FlagToEquip));
		}
	}
	else
	{
		if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
		{
			EquipSecondaryWeapon(WeaponToEquip);
		}
		else
		{
			EquipPrimaryWeapon(WeaponToEquip);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::PickupThrowingGrenades(const int32 Amount)
{
	if (ThrowingGrenades < MaxThrowingGrenades)
	{
		ThrowingGrenades = FMath::Clamp(ThrowingGrenades + Amount, 0, MaxThrowingGrenades);

		if (Character && Character->HasAuthority())
		{
			UpdateHUDThrowingGrenades();
		}
	}
}

void UCombatComponent::LaunchThrowingGrenade()
{
	ShowAttachedGrenade(false);

	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchThrowingGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchThrowingGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && Character->GetAttachedGrenade() && ThrowingGrenadeClass)
	{
		const FVector SpawnLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - SpawnLocation;

		// Calculate the upward component based on the desired angle
		const float AngleInRadians = FMath::DegreesToRadians(ThrowingGrenadeLaunchAngle);
		const float UpwardComponent = FMath::Tan(AngleInRadians) * ToTarget.Size();
		ToTarget.Z += UpwardComponent;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;

		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(
				ThrowingGrenadeClass,
				SpawnLocation,
				ToTarget.Rotation(),
				SpawnParams
			);
		}
	}
}

void UCombatComponent::SwitchAttachedWeapons()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	// Already handled in OnRep methods for clients
	if (Character->HasAuthority())
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		EquippedWeapon->PlayPickupSound();

		if (Character->IsLocallyControlled())
		{
			EquippedWeapon->SetHUDWeaponAmmo();

			PlayerController = PlayerController == nullptr
				                   ? Cast<ABlasterPlayerController>(Character->GetController())
				                   : PlayerController.Get();
			if (PlayerController)
			{
				PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
			}
		}
	}

	AttachActorToSocket(EquippedWeapon, Socket::RightHandSocket);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	const FName SocketName = SecondaryWeapon->GetSecondaryWeaponSocket();
	AttachActorToSocket(SecondaryWeapon, SocketName);
}

void UCombatComponent::PickupAmmo(const EWeaponType WeaponType, const int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		const int32 AmmoToSet = CarriedAmmoMap[WeaponType] + AmmoAmount;
		CarriedAmmoMap[WeaponType] = FMath::Clamp(AmmoToSet, 0, MaxCarriedAmmoMap[WeaponType]);

		UpdateCarriedAmmo();
	}

	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		ReloadEmptyWeapon();
	}
}

bool UCombatComponent::IsCarriedAmmoFull(const EWeaponType WeaponType)
{
	return CarriedAmmoMap[WeaponType] == MaxCarriedAmmoMap[WeaponType];
}

bool UCombatComponent::HasMaxThrowingGrenades() const
{
	return ThrowingGrenades == MaxThrowingGrenades;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		if (const TObjectPtr<UCameraComponent> FollowCamera = Character->GetFollowCamera())
		{
			FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
			FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;

			DefaultFOV = FollowCamera->FieldOfView;
			CurrentFOV = DefaultFOV;
			DefaultFStop = FollowCamera->PostProcessSettings.DepthOfFieldFstop;
			CurrentFStop = DefaultFStop;
			DefaultFocalDistance = FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance;
			CurrentFocalDistance = DefaultFocalDistance;
		}
	}

	RecalculateMovementSpeed();
}

void UCombatComponent::SetHUDCrosshair(const float DeltaTime)
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(Character->GetController())
		                   : PlayerController.Get();
	if (PlayerController)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(PlayerController->GetHUD()) : HUD.Get();
		if (HUD)
		{
			if (EquippedWeapon && !Character->IsLookAroundOnly() && !PlayerController->GetIsEscapeMenuOpen())
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->GetCrosshairCenter();
				HUDPackage.CrosshairLeft = EquippedWeapon->GetCrosshairLeft();
				HUDPackage.CrosshairRight = EquippedWeapon->GetCrosshairRight();
				HUDPackage.CrosshairTop = EquippedWeapon->GetCrosshairTop();
				HUDPackage.CrosshairBottom = EquippedWeapon->GetCrosshairBottom();

				if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle && bIsAiming)
				{
					HUDPackage.CrosshairColor = FLinearColor::Transparent;
				}
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			HUDPackage.CrosshairSpread = CalculateCrosshairSpread(DeltaTime);

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::AimButtonPressed(const bool bPressed)
{
	if (Character == nullptr || EquippedWeapon == nullptr || bIsHoldingFlag)
	{
		return;
	}

	bIsAiming = bPressed;

	// Avoid double calls on the server
	if (!Character->HasAuthority())
	{
		RecalculateMovementSpeed();
	}

	ServerAimButtonPressed(bPressed);

	if (Character->IsLocallyControlled())
	{
		bIsLocallyAiming = bIsAiming;
	}
}

void UCombatComponent::ServerAimButtonPressed_Implementation(const bool bPressed)
{
	bIsAiming = bPressed;

	RecalculateMovementSpeed();
}

void UCombatComponent::FireButtonPressed(const bool bPressed)
{
	bIsFiring = bPressed;

	if (EquippedWeapon && bIsFiring)
	{
		Fire();
	}
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay)
{
	if (EquippedWeapon)
	{
		return FMath::IsNearlyEqual(FireDelay, EquippedWeapon->GetFireDelay(), FIRE_DELAY_ERROR_TOLERANCE);
	}

	return true;
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())
	{
		return;
	}

	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets,
                                                        float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority())
	{
		return;
	}

	LocalShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets,
                                                  const float FireDelay)
{
	if (EquippedWeapon)
	{
		return FMath::IsNearlyEqual(FireDelay, EquippedWeapon->GetFireDelay(), FIRE_DELAY_ERROR_TOLERANCE);
	}

	return true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (Character && EquippedWeapon)
	{
		if (CombatState == ECombatState::ECS_SwitchingWeapons)
		{
			FTimerHandle EquipWeaponTimer;
			Character->GetWorldTimerManager().SetTimer(
				EquipWeaponTimer,
				this,
				&UCombatComponent::HandleEquippedWeaponReplication,
				SWITCH_ATTACHED_WEAPONS_DELAY
			);
		}
		else
		{
			HandleEquippedWeaponReplication();
		}
	}
}

void UCombatComponent::OnRep_SecondaryWeapon() const
{
	if (Character && SecondaryWeapon)
	{
		if (CombatState == ECombatState::ECS_SwitchingWeapons)
		{
			FTimerHandle EquipWeaponTimer;
			Character->GetWorldTimerManager().SetTimer(
				EquipWeaponTimer,
				this,
				&UCombatComponent::HandleSecondaryWeaponReplication,
				SWITCH_ATTACHED_WEAPONS_DELAY
			);
		}
		else
		{
			HandleSecondaryWeaponReplication();
		}
	}
}

void UCombatComponent::HandleEquippedWeaponReplication()
{
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	EquippedWeapon->PlayPickupSound();

	if (Character)
	{
		if (Character->IsLocallyControlled())
		{
			EquippedWeapon->SetHUDWeaponAmmo();

			PlayerController = PlayerController == nullptr
				                   ? Cast<ABlasterPlayerController>(Character->GetController())
				                   : PlayerController.Get();
			if (PlayerController)
			{
				PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
			}
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

	AttachActorToSocket(EquippedWeapon, Socket::RightHandSocket);
}

void UCombatComponent::HandleSecondaryWeaponReplication() const
{
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	if (CombatState != ECombatState::ECS_SwitchingWeapons)
	{
		SecondaryWeapon->PlayPickupSound();
	}

	const FName SocketName = SecondaryWeapon->GetSecondaryWeaponSocket();
	AttachActorToSocket(SecondaryWeapon, SocketName);
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if (Character)
		{
			const float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + TraceStartOffset);
		}

		const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}

		const AActor* HitActor = TraceHitResult.GetActor();
		const bool bInteractsWithCrosshair = HitActor && HitActor->Implements<UInteractWithCrosshairInterface>();

		HUDPackage.CrosshairColor = bInteractsWithCrosshair ? CrosshairOverlapColor : FLinearColor::White;
	}
}

float UCombatComponent::CalculateCrosshairSpread(const float DeltaTime)
{
	check(Character);

	if (EquippedWeapon)
	{
		FVector Velocity = Character->GetVelocity();
		Velocity.Z = 0.f;
		const bool bIsRunning = Velocity.Size() > AimWalkSpeed + 1.f;
		const float InterpSpeed = bIsRunning ? EquippedWeapon->GetZoomOutSpeed() : EquippedWeapon->GetZoomInSpeed();
		const float Target = bIsRunning ? 1.f : 0.f;
		CrosshairVelocityFactor = FMath::FInterpTo(CrosshairVelocityFactor, Target, DeltaTime, InterpSpeed / 3.f);
	}

	if (EquippedWeapon)
	{
		const bool bIsFalling = Character->GetCharacterMovement()->IsFalling();
		const float InterpSpeed = bIsFalling ? EquippedWeapon->GetZoomOutSpeed() : EquippedWeapon->GetZoomInSpeed();
		const float Target = bIsFalling ? 2.f : 0.f;
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, Target, DeltaTime, InterpSpeed);
	}

	if (EquippedWeapon)
	{
		const float InterpSpeed = bIsAiming ? EquippedWeapon->GetZoomInSpeed() : EquippedWeapon->GetZoomOutSpeed();
		const float Target = bIsAiming ? 0.f : 1.f;
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, Target, DeltaTime, InterpSpeed);
	}

	return CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor;
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	CombatState = ECombatState::ECS_Reloading;

	// Already handled in Reload()
	if (!Character->IsLocallyControlled())
	{
		HandleReloadMontage();
	}
}

void UCombatComponent::HandleReloadMontage()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	const FName SectionName = GetSectionNameByWeaponType();
	ReloadDuration = Character->GetReloadDuration(SectionName);
	ReloadStartTime = GetWorld()->GetTimeSeconds();

	if (Character->IsLocallyControlled() && !Character->IsEliminated())
	{
		Character->UpdateHUDReloadProgressVisibility(true);
	}

	Character->GetWorldTimerManager().SetTimer(
		FABRIKBlendInTimer,
		[this] { Character->SetUseFABRIK(false); },
		ReloadDuration * FABRIKReloadBlendPercent,
		false);

	Character->GetWorldTimerManager().SetTimer(
		ReloadMontageTimer,
		this,
		&UCombatComponent::ReloadFinished,
		ReloadDuration,
		false);

	Character->GetWorldTimerManager().SetTimer(
		FABRIKBlendOutTimer,
		[this] { Character->SetUseFABRIK(true); },
		ReloadDuration - ReloadDuration * FABRIKReloadBlendPercent,
		false);

	Character->PlayReloadMontage(SectionName);
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon && CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		const int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
		const int32 AvailableAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		const int32 Least = FMath::Min(RoomInMag, AvailableAmmo);
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

void UCombatComponent::EquipFlag(AFlag* FlagToEquip)
{
	if (Character)
	{
		Character->StopAiming();
	}

	bIsHoldingFlag = true;
	Flag = FlagToEquip;

	Flag->SetOwner(Character);
	Flag->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToSocket(Flag, Socket::FlagSocket);

	RecalculateMovementSpeed();
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	EquippedWeapon->SetHUDWeaponAmmo();
	EquippedWeapon->PlayPickupSound();
	EquippedWeapon->GetOverlapSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AttachActorToSocket(EquippedWeapon, Socket::RightHandSocket);

	UpdateCarriedAmmo();
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	SecondaryWeapon->PlayPickupSound();
	SecondaryWeapon->GetOverlapSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const FName SocketName = SecondaryWeapon->GetSecondaryWeaponSocket();
	AttachActorToSocket(SecondaryWeapon, SocketName);
}

void UCombatComponent::InterpFOV(const float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime,
		                              EquippedWeapon->GetZoomInSpeed());
		CurrentFStop = FMath::FInterpTo(CurrentFStop, 32.f, DeltaTime,
		                                EquippedWeapon->GetZoomInSpeed());
		CurrentFocalDistance = FMath::FInterpTo(CurrentFocalDistance, 10000.f, DeltaTime,
		                                        EquippedWeapon->GetZoomInSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomOutSpeed());
		CurrentFStop = FMath::FInterpTo(CurrentFStop, DefaultFStop, DeltaTime,
		                                EquippedWeapon->GetZoomOutSpeed());
		CurrentFocalDistance = FMath::FInterpTo(CurrentFocalDistance, DefaultFocalDistance, DeltaTime,
		                                        EquippedWeapon->GetZoomOutSpeed());
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
		Character->GetFollowCamera()->PostProcessSettings.DepthOfFieldFstop = CurrentFStop;
		Character->GetFollowCamera()->PostProcessSettings.DepthOfFieldFocalDistance = CurrentFocalDistance;
	}
}

void UCombatComponent::SetSniperScope()
{
	const bool bShouldShowSniperScope = bIsAiming && EquippedWeapon &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bShouldShowSniperScope != bIsSniperScopeVisible)
	{
		Character->ShowSniperScopeWidget(bShouldShowSniperScope);
		bIsSniperScopeVisible = bShouldShowSniperScope;
	}
}

void UCombatComponent::SetReloadProgress()
{
	if (CombatState == ECombatState::ECS_Reloading)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		ReloadProgress = CurrentTime - ReloadStartTime;
		ReloadProgress = FMath::Clamp(ReloadProgress, 0.f, ReloadDuration);
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;

		switch (EquippedWeapon->GetFireType())
		{
		case EFireType::EFT_Hitscan:
			FireHitscanWeapon();
			break;
		case EFireType::EFT_Projectile:
			FireProjectileWeapon();
			break;
		case EFireType::EFT_Shotgun:
			FireShotgunWeapon();
			break;
		case EFireType::EFT_None:
		case EFireType::EFT_MAX:
			break;
		}

		ReloadEmptyWeapon();
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		if (CheckIfWeaponObstructed())
		{
			Character->PlayWeaponObstructionSound();
			return;
		}

		const float AimAccuracyFactor = CalculateAimAccuracyFactor();
		HitTarget = EquippedWeapon->GetUseScatter()
			            ? EquippedWeapon->TraceEndWithScatter(HitTarget, bIsAiming, AimAccuracyFactor)
			            : HitTarget;

		if (!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::FireHitscanWeapon()
{
	if (EquippedWeapon && Character)
	{
		if (CheckIfWeaponObstructed())
		{
			Character->PlayWeaponObstructionSound();
			return;
		}

		const float AimAccuracyFactor = CalculateAimAccuracyFactor();
		HitTarget = EquippedWeapon->GetUseScatter()
			            ? EquippedWeapon->TraceEndWithScatter(HitTarget, bIsAiming, AimAccuracyFactor)
			            : HitTarget;

		if (!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::FireShotgunWeapon()
{
	const AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		if (CheckIfWeaponObstructed())
		{
			Character->PlayWeaponObstructionSound();
			return;
		}

		TArray<FVector_NetQuantize> HitTargets;
		const float AimAccuracyFactor = CalculateAimAccuracyFactor();
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets, bIsAiming, AimAccuracyFactor);

		if (!Character->HasAuthority())
		{
			LocalShotgunFire(HitTargets);
		}
		ServerShotgunFire(HitTargets, EquippedWeapon->GetFireDelay());
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && !Character->IsEliminated() && EquippedWeapon && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Character && !Character->IsEliminated() && Shotgun && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		Shotgun->FireShotgun(TraceHitTargets);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay()
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	bCanFire = true;
	if (EquippedWeapon->GetIsAutomatic() && bIsFiring)
	{
		Fire();
	}
}

bool UCombatComponent::CanFire() const
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	return !EquippedWeapon->IsEmpty() && CombatState == ECombatState::ECS_Unoccupied && bCanFire && !bIsHoldingFlag;
}

bool UCombatComponent::CheckIfWeaponObstructed()
{
	if (Character && EquippedWeapon && EquippedWeapon->GetWeaponMesh())
	{
		const FVector CharacterCenterLocation = Character->GetActorLocation();
		const FVector MuzzleFlashLocation = EquippedWeapon->GetWeaponMesh()->GetSocketLocation(Socket::MuzzleFlash);

		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(Character);
		CollisionParams.AddIgnoredActor(EquippedWeapon);

		GetWorld()->SweepSingleByChannel(
			HitResult,
			CharacterCenterLocation,
			MuzzleFlashLocation,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(SWEEP_SPHERE_RADIUS),
			CollisionParams
		);

		if (HitResult.bBlockingHit && HitResult.GetActor() && !Cast<ABlasterCharacter>(HitResult.GetActor()))
		{
			return true;
		}
	}
	return false;
}

void UCombatComponent::Reload()
{
	const bool bIsFull = EquippedWeapon && EquippedWeapon->IsFull();
	const bool bIsEmpty = EquippedWeapon && EquippedWeapon->IsEmpty();
	const bool bLockManualReload = bIsEmpty && bAwaitingEmptyReload;
	const bool bCannotReload = bIsFull || bLockManualReload || CarriedAmmo == 0 ||
		CombatState != ECombatState::ECS_Unoccupied || bIsHoldingFlag;
	if (bCannotReload)
	{
		return;
	}

	// Client-side prediction
	if (Character && Character->IsLocallyControlled())
	{
		CombatState = ECombatState::ECS_Reloading;
		HandleReloadMontage();
	}

	ServerReload();
}

void UCombatComponent::ReloadFinished()
{
	if (Character && Character->IsEliminated())
	{
		return;
	}

	ReloadProgress = 0.f;

	if (Character && Character->IsLocallyControlled())
	{
		Character->UpdateHUDReloadProgressVisibility(false);
	}

	// Client-side prediction
	if (Character && Character->IsLocallyControlled())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}

	// Server authoritative
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	const int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];

		PlayerController = PlayerController == nullptr
			                   ? Cast<ABlasterPlayerController>(Character->GetController())
			                   : PlayerController.Get();
		if (PlayerController)
		{
			PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}

	EquippedWeapon->AddAmmo(ReloadAmount);

	if (bIsFiring)
	{
		Fire();
	}
}

void UCombatComponent::ThrowGrenade()
{
	const bool bCannotThrowGrenade = EquippedWeapon == nullptr ||
		CombatState != ECombatState::ECS_Unoccupied || ThrowingGrenades == 0 || bIsHoldingFlag;
	if (bCannotThrowGrenade)
	{
		return;
	}

	CombatState = ECombatState::ECS_ThrowingGrenade;

	HandleThrowGrenadeMontage();

	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}

	if (Character && Character->HasAuthority())
	{
		ThrowingGrenades = FMath::Clamp(ThrowingGrenades - 1, 0, MaxThrowingGrenades);
		UpdateHUDThrowingGrenades();
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		ReloadEmptyWeapon();
	}

	if (bIsFiring)
	{
		Fire();
	}
}

void UCombatComponent::SwitchWeapons()
{
	const bool bCannotSwitchWeapons = Character == nullptr || EquippedWeapon == nullptr ||
		SecondaryWeapon == nullptr || CombatState != ECombatState::ECS_Unoccupied || bIsHoldingFlag;
	if (bCannotSwitchWeapons)
	{
		return;
	}

	CombatState = ECombatState::ECS_SwitchingWeapons;
	HandleSwitchWeaponsMontage();

	// Delay the weapon switch to ensure the FABRIK blend is over
	Character->GetWorldTimerManager().SetTimer(
		SwitchWeaponsTimer,
		[this]
		{
			AWeapon* TempWeapon = EquippedWeapon;
			EquippedWeapon = SecondaryWeapon;
			SecondaryWeapon = TempWeapon;

			UpdateCarriedAmmo(false); // Early call to replicate carried ammo in time for HUD update on clients
		},
		Character->GetFABRIKBlendTime(),
		false
	);
}

void UCombatComponent::SwitchWeaponsFinished()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		ReloadEmptyWeapon();
	}

	if (bIsFiring)
	{
		Fire();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (ThrowingGrenades == 0)
	{
		return;
	}

	CombatState = ECombatState::ECS_ThrowingGrenade;

	HandleThrowGrenadeMontage();

	ThrowingGrenades = FMath::Clamp(ThrowingGrenades - 1, 0, MaxThrowingGrenades);
	UpdateHUDThrowingGrenades();
}

void UCombatComponent::UpdateHUDThrowingGrenades()
{
	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(Character->GetController())
		                   : PlayerController.Get();
	if (PlayerController)
	{
		PlayerController->SetHUDGrenades(ThrowingGrenades);
	}
}

void UCombatComponent::OnRep_ThrowingGrenades()
{
	UpdateHUDThrowingGrenades();
}

FName UCombatComponent::GetSectionNameByWeaponType() const
{
	FName SectionName;

	switch (EquippedWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_AssaultRifle:
		SectionName = MontageSection::AssaultRifle;
		break;
	case EWeaponType::EWT_RocketLauncher:
		SectionName = MontageSection::RocketLauncher;
		break;
	case EWeaponType::EWT_Pistol:
		SectionName = MontageSection::Pistol;
		break;
	case EWeaponType::EWT_SMG:
		SectionName = MontageSection::SMG;
		break;
	case EWeaponType::EWT_Shotgun:
		SectionName = MontageSection::Shotgun;
		break;
	case EWeaponType::EWT_SniperRifle:
		SectionName = MontageSection::SniperRifle;
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		SectionName = MontageSection::GrenadeLauncher;
		break;
	default: ;
	}

	return SectionName;
}

void UCombatComponent::DropEquippedWeapon() const
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::DropSecondaryWeapon() const
{
	if (SecondaryWeapon)
	{
		SecondaryWeapon->Dropped();
	}
}

void UCombatComponent::DropFlag(const bool bForceDrop)
{
	if (Flag && (bForceDrop || Flag->CanBeDropped(true)))
	{
		Flag->Dropped();
		bIsHoldingFlag = false;

		if (Character && EquippedWeapon)
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->bUseControllerRotationYaw = true;
		}

		RecalculateMovementSpeed();
	}
}

void UCombatComponent::AttachActorToSocket(AActor* ActorToAttach, const FName SocketName) const
{
	if (Character == nullptr || ActorToAttach == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (const USkeletalMeshSocket* Socket = CharacterMesh->GetSocketByName(SocketName))
	{
		Socket->AttachActor(ActorToAttach, CharacterMesh);
	}
}

void UCombatComponent::UpdateCarriedAmmo(const bool bUpdateHUD)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(Character->GetController())
		                   : PlayerController.Get();
	if (PlayerController && bUpdateHUD)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	ClearReloadEmptyWeaponTimer();

	if (Character)
	{
		bAwaitingEmptyReload = true;
		Character->GetWorldTimerManager().SetTimer(
			ReloadEmptyWeaponTimer,
			[this]
			{
				bAwaitingEmptyReload = false;
				if (EquippedWeapon && EquippedWeapon->IsEmpty())
				{
					Reload();
				}
			},
			ReloadEmptyWeaponDelay,
			false
		);
	}
}

void UCombatComponent::ClearReloadEmptyWeaponTimer()
{
	if (Character && Character->GetWorldTimerManager().IsTimerActive(ReloadEmptyWeaponTimer))
	{
		Character->GetWorldTimerManager().ClearTimer(ReloadEmptyWeaponTimer);
		bAwaitingEmptyReload = false;
	}
}

void UCombatComponent::ShowAttachedGrenade(const bool bShowGrenade) const
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::OnRep_IsAiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bIsAiming = bIsLocallyAiming;
	}

	RecalculateMovementSpeed();
}

float UCombatComponent::CalculateAimAccuracyFactor() const
{
	if (!bIsAiming || DefaultFOV == CurrentFOV || EquippedWeapon == nullptr)
	{
		return 0.f;
	}

	const float ZoomedFOV = EquippedWeapon->GetZoomedFOV();
	const float ZoomProgress = (DefaultFOV - CurrentFOV) / (DefaultFOV - ZoomedFOV);

	return FMath::Clamp(ZoomProgress, 0.f, 1.f);
}

void UCombatComponent::RecalculateMovementSpeed()
{
	if (Character && Character->GetCharacterMovement())
	{
		if (bIsHoldingFlag)
		{
			const float MaxWalkSpeedToSet = FlagWalkSpeed * SpeedMultiplier;
			Character->GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedToSet;
		}
		else
		{
			const float MaxWalkSpeedToSet = (bIsAiming ? AimWalkSpeed : BaseWalkSpeed) * SpeedMultiplier;
			Character->GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeedToSet;
		}

		const float MaxCrouchWalkSpeedToSet = CrouchWalkSpeed * SpeedMultiplier;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = MaxCrouchWalkSpeedToSet;
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	if (CombatState == ECombatState::ECS_SwitchingWeapons)
	{
		// Switching weapons handles updating the HUD separately
		return;
	}

	PlayerController = PlayerController == nullptr
		                   ? Cast<ABlasterPlayerController>(Character->GetController())
		                   : PlayerController.Get();
	if (PlayerController)
	{
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}

	if (bIsFiring)
	{
		Fire();
	}
}

void UCombatComponent::HandleThrowGrenadeMontage()
{
	if (Character == nullptr)
	{
		return;
	}

	const float ThrowGrenadeDuration = Character->GetThrowGrenadeDuration();

	Character->GetWorldTimerManager().SetTimer(
		FABRIKBlendInTimer,
		[this] { Character->SetUseFABRIK(false); },
		ThrowGrenadeDuration * FABRIKThrowGrenadeBlendPercent,
		false
	);

	Character->GetWorldTimerManager().SetTimer(
		ThrowGrenadeMontageTimer,
		this,
		&UCombatComponent::ThrowGrenadeFinished,
		ThrowGrenadeDuration,
		false
	);

	Character->GetWorldTimerManager().SetTimer(
		FABRIKBlendOutTimer,
		[this] { Character->SetUseFABRIK(true); },
		ThrowGrenadeDuration - ThrowGrenadeDuration * FABRIKThrowGrenadeBlendPercent,
		false
	);

	Character->PlayThrowGrenadeMontage();

	Character->GetWorldTimerManager().SetTimer(
		ShowAttachedGrenadeDelayTimer,
		[this] { ShowAttachedGrenade(true); },
		ShowAttachedGrenadeDelay,
		false
	);
}

void UCombatComponent::HandleSwitchWeaponsMontage()
{
	if (Character == nullptr)
	{
		return;
	}

	const float SwitchWeaponsDuration = Character->GetSwitchWeaponsDuration();

	Character->SetUseFABRIK(false);

	Character->GetWorldTimerManager().SetTimer(
		SwitchWeaponsMontageTimer,
		this,
		&UCombatComponent::SwitchWeaponsFinished,
		SwitchWeaponsDuration,
		false
	);

	Character->GetWorldTimerManager().SetTimer(
		FABRIKBlendOutTimer,
		[this] { Character->SetUseFABRIK(true); },
		SwitchWeaponsDuration - SwitchWeaponsDuration * FABRIKSwitchWeaponsBlendPercent,
		false
	);

	Character->PlaySwitchWeaponsMontage();
}

void UCombatComponent::OnRep_CombatState()
{
	// No need to check for locally controlled, client-side prediction prevents this from being called on client
	// This prevents the locally controlled client from handling the montages twice
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		if (bIsFiring)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		HandleReloadMontage();
		break;
	case ECombatState::ECS_ThrowingGrenade:
		HandleThrowGrenadeMontage();
		break;
	case ECombatState::ECS_SwitchingWeapons:
		HandleSwitchWeaponsMontage();
		break;
	case ECombatState::ECS_MAX:
		break;
	default: ;
	}
}

void UCombatComponent::OnRep_IsHoldingFlag()
{
	RecalculateMovementSpeed();
}
