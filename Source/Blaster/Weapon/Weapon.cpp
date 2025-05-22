// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "AmmoCasing.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(true);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetSphereRadius(96.f);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidgetComponent->SetupAttachment(RootComponent);
	PickupWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PickupWidgetComponent->SetDrawAtDesiredSize(true);

	SetCanBeDamaged(false);
}

void AWeapon::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleHovering(DeltaTime);

	if (WeaponState == EWeaponState::EWS_Dropped)
	{
		SetPickupWidgetComponentPosition();
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName(Socket::MuzzleFlash))
	{
		PlayFireEffects(MuzzleFlashSocket);
	}

	if (AmmoCasingClass)
	{
		if (const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(Socket::AmmoEject))
		{
			const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			if (UWorld* World = GetWorld())
			{
				World->SpawnActor<AAmmoCasing>(
					AmmoCasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	SpendRound();
}

bool AWeapon::IsFalling() const
{
	return bIsFalling;
}

bool AWeapon::CanBeDropped(bool bPerformTrace)
{
	return false;
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	const FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
	SetOwner(nullptr);
}

void AWeapon::ShowPickupWidget(const bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

FVector AWeapon::GetPickupSocketLocation() const
{
	if (WeaponMesh)
	{
		return WeaponMesh->GetSocketLocation(Socket::PickupSocket);
	}

	return FVector::ZeroVector;
}

FVector AWeapon::GetMuzzleFlashSocketLocation() const
{
	if (WeaponMesh)
	{
		return WeaponMesh->GetSocketLocation(Socket::MuzzleFlash);
	}

	return FVector::ZeroVector;
}

void AWeapon::SetWeaponState(const EWeaponState State)
{
	WeaponState = State;

	OnWeaponStateSet();
}

void AWeapon::SetHUDWeaponAmmo()
{
	UpdateOwnersIfNecessary();

	if (OwnerController)
	{
		OwnerController->SetHUDWeaponAmmo(Ammo, AmmoColor);
	}
}

void AWeapon::SetHUDCarriedAmmo()
{
	UpdateOwnersIfNecessary();

	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(Ammo);
	}
}

void AWeapon::PlayPickupSound()
{
	if (PickupSound)
	{
		UGameplayStatics::SpawnSoundAttached(PickupSound, GetRootComponent());
	}
}

void AWeapon::AddAmmo(const int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);

	SetHUDWeaponAmmo();
	MulticastAddAmmo(AmmoToAdd);
}

void AWeapon::EnableCustomDepth(const bool bEnable) const
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

FName AWeapon::GetSecondaryWeaponSocket() const
{
	switch (WeaponType)
	{
	case EWeaponType::EWT_Pistol:
	case EWeaponType::EWT_AssaultRifle:
	case EWeaponType::EWT_SniperRifle:
		return Socket::BackpackSocket;
	case EWeaponType::EWT_RocketLauncher:
		return Socket::RocketLauncherBackpackSocket;
	case EWeaponType::EWT_SMG:
		return Socket::SMGBackpackSocket;
	case EWeaponType::EWT_Shotgun:
	case EWeaponType::EWT_GrenadeLauncher:
		return Socket::ShotgunBackpackSocket;
	default:
		return Socket::BackpackSocket;
	}
}

void AWeapon::SpawnDamageBuffEffect(UNiagaraSystem* DamageBuffEffect)
{
	PendingSpawnDamageBuffEffect = false;

	if (DamageBuffEffect)
	{
		const bool bDamageBuffEffectAlreadyActive = DamageBuffEffectComponent && DamageBuffEffectComponent->IsActive();
		if (!bDamageBuffEffectAlreadyActive)
		{
			FRotator RotationWithOffset = FRotator::ZeroRotator;
			RotationWithOffset.Roll = DamageBuffEffectRollOffset;

			DamageBuffEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				DamageBuffEffect,
				GetRootComponent(),
				Socket::MuzzleFlash,
				FVector::ZeroVector,
				RotationWithOffset,
				EAttachLocation::KeepRelativeOffset,
				true
			);
		}
	}
	else if (DamageBuffEffectComponent)
	{
		DamageBuffEffectComponent->DeactivateImmediate();
	}
}

void AWeapon::SetDamageBuffEffectVisibility(const bool bNewVisibility) const
{
	if (DamageBuffEffectComponent)
	{
		DamageBuffEffectComponent->SetVisibility(bNewVisibility);
	}
}

float AWeapon::CalculateDamageWithFalloff(const float BaseDamage, const float Distance) const
{
	if (!bUseDamageFalloff || Distance <= DamageFalloffStartDistance)
	{
		return BaseDamage;
	}

	if (Distance >= DamageFalloffEndDistance)
	{
		return BaseDamage * MinDamagePercentAfterFalloff;
	}

	const float FalloffRange = DamageFalloffEndDistance - DamageFalloffStartDistance;
	const float DistanceIntoFalloffRange = Distance - DamageFalloffStartDistance;
	const float FalloffPercent = FMath::Clamp(DistanceIntoFalloffRange / FalloffRange, 0.f, 1.f);
	const float DamageMultiplier = FMath::Lerp(1.f, MinDamagePercentAfterFalloff, FalloffPercent);

	return BaseDamage * DamageMultiplier;
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget, const bool bIsAiming,
                                     const float AimAccuracyFactor) const
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(Socket::MuzzleFlash);
	if (MuzzleFlashSocket == nullptr)
	{
		return FVector();
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
	const FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLocation = SphereCenter + RandVector;
	const FVector ToEndLocation = EndLocation - TraceStart;

	return FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapSphere)
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(
				CheckForOverlappingActorsTimer,
				this,
				&AWeapon::CheckForOverlappingActors,
				CheckForOverlappingActorsInterval,
				true,
				CheckForOverlappingActorsInterval
			);
		}
	}

	SetPickupWidget();

	if (WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue(WeaponCustomDepthStencilValue);
		WeaponMesh->MarkRenderStateDirty();

		SetPickupWidgetComponentPosition();

		if (HasAuthority())
		{
			WeaponMesh->OnComponentHit.AddDynamic(this, &AWeapon::OnComponentHit);
		}
	}

	EnableCustomDepth(true);
}

void AWeapon::OnWeaponStateSet()
{
	UpdateOwnersIfNecessary();

	switch (WeaponState)
	{
	case EWeaponState::EWS_Initial:
		HandleWeaponInitialState();
		break;
	case EWeaponState::EWS_Equipped:
		HandleWeaponEquippedState();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		HandleWeaponEquippedSecondaryState();
		break;
	case EWeaponState::EWS_Dropped:
		HandleWeaponDroppedState();
		break;
	default: ;
	}
}

void AWeapon::HandleWeaponInitialState()
{
	SetReplicateMovement(false);
	SetPickupWidgetComponentPosition();
	bShouldHover = true;
}

void AWeapon::HandleWeaponEquippedState()
{
	SetReplicateMovement(false);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bEnableWeaponPhysics)
	{
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	ShowPickupWidget(false);
	EnableCustomDepth(false);

	if (OwnerCharacter)
	{
		SpawnDamageBuffEffect(OwnerCharacter->GetDamageBuffEffect());
	}
	else
	{
		PendingSpawnDamageBuffEffect = true;
	}

	WeaponMesh->SetAllBodiesNotifyRigidBodyCollision(false);
	WeaponMesh->SetUseCCD(false);

	bShouldHover = false;

	if (HasAuthority())
	{
		OnPickedUp.Broadcast();
		StopDestroyWeaponTimer();
	}
}

void AWeapon::HandleWeaponEquippedSecondaryState()
{
	SetReplicateMovement(false);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bEnableWeaponPhysics)
	{
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	WeaponMesh->SetAllBodiesNotifyRigidBodyCollision(false);
	WeaponMesh->SetUseCCD(false);

	ShowPickupWidget(false);
	EnableCustomDepth(false);
	SpawnDamageBuffEffect(nullptr);

	bShouldHover = false;

	if (HasAuthority())
	{
		OnPickedUp.Broadcast();
		StopDestroyWeaponTimer();
	}
}

void AWeapon::HandleWeaponDroppedState()
{
	SetReplicateMovement(true);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	WeaponMesh->SetAllBodiesNotifyRigidBodyCollision(true);
	WeaponMesh->SetUseCCD(true);

	WeaponMesh->SetCustomDepthStencilValue(WeaponCustomDepthStencilValue);
	WeaponMesh->MarkRenderStateDirty();

	EnableCustomDepth(true);
	SpawnDamageBuffEffect(nullptr);

	bShouldHover = false;

	if (HasAuthority())
	{
		bIsFalling = true;
		StartDestroyWeaponTimer();
	}
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		const bool bIsTeamFlag = WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() == Team;
		if (bIsTeamFlag)
		{
			return;
		}
		BlasterCharacter->AddOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		const bool bIsTeamFlag = WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() == Team;
		if (bIsTeamFlag)
		{
			return;
		}
		BlasterCharacter->RemoveOverlappingWeapon(this);
	}
}

void AWeapon::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                             FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsFalling)
	{
		bIsFalling = false;

		if (DropSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DropSound, GetActorLocation());
		}
	}
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
	else
	{
		OwnerCharacter = OwnerCharacter == nullptr
			                 ? Cast<ABlasterCharacter>(Owner)
			                 : OwnerCharacter.Get();

		if (OwnerCharacter && OwnerCharacter->GetEquippedWeapon() && OwnerCharacter->
			GetEquippedWeapon() == this)
		{
			SetHUDWeaponAmmo();
		}

		if (PendingSpawnDamageBuffEffect)
		{
			SpawnDamageBuffEffect(OwnerCharacter->GetDamageBuffEffect());
		}
	}
}

void AWeapon::SetPickupWidget()
{
	if (PickupWidgetComponent)
	{
		PickupWidget = Cast<UUserWidget>(PickupWidgetComponent->GetWidget());
	}
}

void AWeapon::SetPickupWidgetComponentPosition() const
{
	if (PickupWidgetComponent)
	{
		PickupWidgetComponent->SetWorldLocation(GetPickupSocketLocation());
	}
}

void AWeapon::PlayFireEffects(const USkeletalMeshSocket* MuzzleFlashSocket) const
{
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);

	if (MuzzleFlashParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			this,
			MuzzleFlashParticles,
			SocketTransform.GetLocation(),
			SocketTransform.GetRotation().Rotator()
		);
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::CheckForOverlappingActors()
{
	check(OverlapSphere);

	if (!OverlapSphere->OnComponentBeginOverlap.IsAlreadyBound(this, &AWeapon::OnSphereBeginOverlap))
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereBeginOverlap);
	}

	if (!OverlapSphere->OnComponentEndOverlap.IsAlreadyBound(this, &AWeapon::OnSphereEndOverlap))
	{
		OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	TArray<AActor*> OverlappingActors;
	OverlapSphere->GetOverlappingActors(OverlappingActors, ABlasterCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(Actor))
		{
			OnSphereBeginOverlap(
				OverlapSphere,
				OverlappingCharacter,
				nullptr,
				0,
				false,
				FHitResult()
			);
			break;
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);

	SetHUDWeaponAmmo();

	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		Sequence++;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(const int32 ServerAmmo)
{
	if (HasAuthority())
	{
		return;
	}

	Ammo = ServerAmmo;
	Sequence--;
	Ammo -= Sequence;

	SetHUDWeaponAmmo();
}

void AWeapon::MulticastAddAmmo_Implementation(const int32 AmmoToAdd)
{
	if (HasAuthority())
	{
		return;
	}

	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);

	SetHUDWeaponAmmo();
}

void AWeapon::UpdateOwnersIfNecessary()
{
	if (OwnerCharacter == nullptr)
	{
		OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	}

	if (OwnerController == nullptr && OwnerCharacter)
	{
		OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->GetController());
	}
}

void AWeapon::HandleHovering(const float DeltaTime)
{
	RunningTime += DeltaTime;

	if (WeaponMesh && bShouldHover)
	{
		const double TransformedSin = FMath::Sin(RunningTime * TimeConstant) * Amplitude;
		WeaponMesh->AddWorldOffset(FVector(0.f, 0.f, TransformedSin));
	}
}

void AWeapon::StartDestroyWeaponTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyWeaponTimer,
		this,
		&AWeapon::DestroyWeapon,
		DestroyWeaponTime,
		false
	);
}

void AWeapon::StopDestroyWeaponTimer()
{
	GetWorldTimerManager().ClearTimer(DestroyWeaponTimer);
}

void AWeapon::DestroyWeapon()
{
	GetWorldTimerManager().ClearTimer(CheckForOverlappingActorsTimer);
	Destroy();
}
