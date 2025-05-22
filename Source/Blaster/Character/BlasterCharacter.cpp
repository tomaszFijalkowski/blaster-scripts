// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameMode/LobbyGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/GameUserSettings/BlasterGameUserSettings.h"
#include "Blaster/HUD/DamageWidget.h"
#include "Blaster/HUD/OverheadWidget.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/BodyshotDamageType.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Zone/SpawnZone.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"

class UBlasterGameUserSettings;

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);

	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	DamageWidgetComponent->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComponent->SetIsReplicated(true);

	LagCompensationComponent = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 120.f;
	MinNetUpdateFrequency = 120.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), Socket::GrenadeSocket);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/**
	 * Hitboxes used for server-side rewind
	 */
	HeadBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadBox"));
	HeadBox->SetupAttachment(GetMesh(), Bone::HeadBone);
	Hitboxes.Add(Bone::HeadBone, HeadBox);

	PelvisBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PelvisBox"));
	PelvisBox->SetupAttachment(GetMesh(), Bone::PelvisBone);
	Hitboxes.Add(Bone::PelvisBone, PelvisBox);

	Spine1Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine1Box"));
	Spine1Box->SetupAttachment(GetMesh(), Bone::Spine1Bone);
	Hitboxes.Add(Bone::Spine1Bone, Spine1Box);

	Spine2Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine2Box"));
	Spine2Box->SetupAttachment(GetMesh(), Bone::Spine2Bone);
	Hitboxes.Add(Bone::Spine2Bone, Spine2Box);

	Spine3Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine3Box"));
	Spine3Box->SetupAttachment(GetMesh(), Bone::Spine3Bone);
	Hitboxes.Add(Bone::Spine3Bone, Spine3Box);

	LeftUpperArmBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftUpperArmBox"));
	LeftUpperArmBox->SetupAttachment(GetMesh(), Bone::LeftUpperArmBone);
	Hitboxes.Add(Bone::LeftUpperArmBone, LeftUpperArmBox);

	RightUpperArmBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightUpperArmBox"));
	RightUpperArmBox->SetupAttachment(GetMesh(), Bone::RightUpperArmBone);
	Hitboxes.Add(Bone::RightUpperArmBone, RightUpperArmBox);

	LeftLowerArmBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftLowerArmBox"));
	LeftLowerArmBox->SetupAttachment(GetMesh(), Bone::LeftLowerArmBone);
	Hitboxes.Add(Bone::LeftLowerArmBone, LeftLowerArmBox);

	RightLowerArmBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightLowerArmBox"));
	RightLowerArmBox->SetupAttachment(GetMesh(), Bone::RightLowerArmBone);
	Hitboxes.Add(Bone::RightLowerArmBone, RightLowerArmBox);

	LeftHandBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftHandBox"));
	LeftHandBox->SetupAttachment(GetMesh(), Bone::LeftHandBone);
	Hitboxes.Add(Bone::LeftHandBone, LeftHandBox);

	RightHandBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightHandBox"));
	RightHandBox->SetupAttachment(GetMesh(), Bone::RightHandBone);
	Hitboxes.Add(Bone::RightHandBone, RightHandBox);

	LeftThighBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftThighBox"));
	LeftThighBox->SetupAttachment(GetMesh(), Bone::LeftThighBone);
	Hitboxes.Add(Bone::LeftThighBone, LeftThighBox);

	RightThighBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightThighBox"));
	RightThighBox->SetupAttachment(GetMesh(), Bone::RightThighBone);
	Hitboxes.Add(Bone::RightThighBone, RightThighBox);

	LeftCalfBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftCalfBox"));
	LeftCalfBox->SetupAttachment(GetMesh(), Bone::LeftCalfBone);
	Hitboxes.Add(Bone::LeftCalfBone, LeftCalfBox);

	RightCalfBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightCalfBox"));
	RightCalfBox->SetupAttachment(GetMesh(), Bone::RightCalfBone);
	Hitboxes.Add(Bone::RightCalfBone, RightCalfBox);

	LeftFootBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFootBox"));
	LeftFootBox->SetupAttachment(GetMesh(), Bone::LeftFootBone);
	Hitboxes.Add(Bone::LeftFootBone, LeftFootBox);

	RightFootBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFootBox"));
	RightFootBox->SetupAttachment(GetMesh(), Bone::RightFootBone);
	Hitboxes.Add(Bone::RightFootBone, RightFootBox);

	BackpackBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BackpackBox"));
	BackpackBox->SetupAttachment(GetMesh(), Bone::BackpackBone);
	Hitboxes.Add(Bone::BackpackBone, BackpackBox);

	BlanketBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlanketBox"));
	BlanketBox->SetupAttachment(GetMesh(), Bone::BackpackBone);
	Hitboxes.Add(Bone::BackpackBone, BlanketBox);

	for (auto Hitbox : Hitboxes)
	{
		if (Hitbox.Value)
		{
			Hitbox.Value->SetCollisionObjectType(ECC_Hitbox);
			Hitbox.Value->SetCollisionResponseToAllChannels(ECR_Ignore);
			Hitbox.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
			Hitbox.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasterCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);

	HideCharacterIfCameraTooClose();
	PollPlayerStateInit();
	PollSettingTeamColor();
	PollSettingOverheadWidget();
	PollSettingDamageWidget();

	UpdateHUDReloadProgress();
	UpdateHUDBuffDurations();
	UpdateOverheadWidgetDistance();
	UpdateDamageWidgetDistance();

	SetShieldEffectPosition();
	SetCrownEffectPosition();
	SetOverheadWidgetPosition();
	SetDamageWidgetPosition();

	if (HasAuthority())
	{
		DetermineWeaponToPickup();
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CustomJump);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterCharacter::CustomCrouch);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Equip);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Aim);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Fire);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Reload);
		EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterCharacter::ThrowGrenade);
		EnhancedInputComponent->BindAction(SwitchWeaponsAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterCharacter::SwitchWeapons);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, WeaponToPickup, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CombatComponent)
	{
		CombatComponent->Character = this;

		if (BuffComponent)
		{
			BuffComponent->Character = this;
			BuffComponent->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
		}
	}

	if (LagCompensationComponent)
	{
		LagCompensationComponent->Character = this;

		if (PlayerController)
		{
			LagCompensationComponent->PlayerController = PlayerController;
		}
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();

	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PlayerController = Cast<ABlasterPlayerController>(NewController);
	if (PlayerController)
	{
		PlayerController->PlayerCameraManager->ViewPitchMin = -VIEW_PITCH_LIMIT;
		PlayerController->PlayerCameraManager->ViewPitchMax = VIEW_PITCH_LIMIT;

		const int32 MaxThrowingGrenades = CombatComponent->GetThrowingGrenades();
		PlayerController->SetHUDGrenades(MaxThrowingGrenades);
	}

	if (IsLocallyControlled())
	{
		DefaultFOV = FollowCamera ? FollowCamera->FieldOfView : DEFAULT_FOV;

		if (GEngine)
		{
			GameUserSettings = Cast<UBlasterGameUserSettings>(GEngine->GetGameUserSettings());
		}
	}

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (EliminationBotComponent)
	{
		EliminationBotComponent->DestroyComponent();
	}

	BlasterGameMode = BlasterGameMode ? BlasterGameMode.Get() : GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	const bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (CombatComponent && CombatComponent->EquippedWeapon && bMatchNotInProgress)
	{
		CombatComponent->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::DetermineWeaponToPickup()
{
	float MinDistance = FLT_MAX;
	AWeapon* DeterminedWeaponToPickup = nullptr;

	if (IsHoldingFlag())
	{
		return;
	}

	// Find closest overlapping weapon that is not falling
	for (AWeapon* OverlappingWeapon : OverlappingWeapons)
	{
		if (OverlappingWeapon && !OverlappingWeapon->IsFalling())
		{
			const float Distance = FVector::Dist(
				GetMesh()->GetSocketLocation(Socket::RightHandSocket),
				OverlappingWeapon->GetPickupSocketLocation()
			);

			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				DeterminedWeaponToPickup = OverlappingWeapon;
			}
		}
	}

	AWeapon* PreviousWeaponToPickup = WeaponToPickup;
	if (DeterminedWeaponToPickup != PreviousWeaponToPickup)
	{
		WeaponToPickup = DeterminedWeaponToPickup;

		if (IsLocallyControlled())
		{
			if (WeaponToPickup)
			{
				WeaponToPickup->ShowPickupWidget(true);
			}

			if (PreviousWeaponToPickup)
			{
				PreviousWeaponToPickup->ShowPickupWidget(false);
			}
		}
	}
}

void ABlasterCharacter::AddOverlappingWeapon(AWeapon* Weapon)
{
	if (Weapon)
	{
		OverlappingWeapons.Add(Weapon);
		DetermineWeaponToPickup();
	}
}

void ABlasterCharacter::RemoveOverlappingWeapon(AWeapon* Weapon)
{
	if (Weapon)
	{
		OverlappingWeapons.Remove(Weapon);
		DetermineWeaponToPickup();
	}
}

TObjectPtr<AWeapon> ABlasterCharacter::GetEquippedWeapon() const
{
	if (CombatComponent == nullptr)
	{
		return nullptr;
	}

	return CombatComponent->EquippedWeapon;
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return CombatComponent && CombatComponent->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->bIsAiming;
}

bool ABlasterCharacter::IsHoldingFlag() const
{
	return CombatComponent && CombatComponent->bIsHoldingFlag;
}

void ABlasterCharacter::SetHoldingFlag(const bool bHolding) const
{
	if (CombatComponent)
	{
		CombatComponent->bIsHoldingFlag = bHolding;
	}
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState ? BlasterPlayerState.Get() : GetPlayerState<ABlasterPlayerState>();
	return BlasterPlayerState ? BlasterPlayerState->GetTeam() : ETeam::ET_NoTeam;
}

void ABlasterCharacter::PlayFireMontage(const bool bIsAiming) const
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bIsAiming ? MontageSection::RifleAim : MontageSection::RifleHip;
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage(const FName SectionName) const
{
	if (CombatComponent == nullptr || bIsEliminated)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::StopReloadMontage() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Stop(0.f, ReloadMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwitchWeaponsMontage() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwitchWeaponsMontage)
	{
		AnimInstance->Montage_Play(SwitchWeaponsMontage);
	}
}

void ABlasterCharacter::PlayWeaponObstructionSound() const
{
	if (WeaponObstructionSound && WeaponObstructionSoundAttenuation)
	{
		UGameplayStatics::SpawnSoundAttached(
			WeaponObstructionSound,
			GetRootComponent(),
			NAME_None,
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			WeaponObstructionSoundAttenuation,
			nullptr,
			false
		);
	}
}

float ABlasterCharacter::GetReloadDuration(const FName SectionName) const
{
	if (ReloadMontage)
	{
		const int32 SectionIndex = ReloadMontage->GetSectionIndex(SectionName);
		return ReloadMontage->GetSectionLength(SectionIndex);
	}
	return 0.f;
}

float ABlasterCharacter::GetThrowGrenadeDuration() const
{
	if (ThrowGrenadeMontage)
	{
		const int32 SectionIndex = ThrowGrenadeMontage->GetSectionIndex(MontageSection::Default);
		return ThrowGrenadeMontage->GetSectionLength(SectionIndex);
	}
	return 0.f;
}

float ABlasterCharacter::GetSwitchWeaponsDuration() const
{
	if (SwitchWeaponsMontage)
	{
		const int32 SectionIndex = SwitchWeaponsMontage->GetSectionIndex(MontageSection::Default);
		return SwitchWeaponsMontage->GetSectionLength(SectionIndex);
	}
	return 0.f;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	return CombatComponent ? CombatComponent->HitTarget : FVector();
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	return CombatComponent ? CombatComponent->CombatState : ECombatState::ECS_MAX;
}

void ABlasterCharacter::Eliminate()
{
	if (CombatComponent)
	{
		CombatComponent->DropEquippedWeapon();
		CombatComponent->DropSecondaryWeapon();
		CombatComponent->DropFlag(true);
	}

	MulticastEliminate();
}

void ABlasterCharacter::MulticastEliminate_Implementation()
{
	if (PlayerController)
	{
		const bool bShouldUseFallbackSpawn = IsInsideSpawnZone() && GetMostRecentDamager() != nullptr;
		PlayerController->SetShouldUseFallbackSpawn(bShouldUseFallbackSpawn);
		PlayerController->SetHUDWeaponAmmo(0);
	}

	bIsEliminated = true;
	PlayEliminationMontage();

	// Start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(MaterialParameter::Dissolve, 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(MaterialParameter::Glow, 200.f);
	}

	StartDissolve();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (IsLocallyControlled())
	{
		UpdateHUDReloadProgressVisibility(false);
		EnableLookAroundOnly();
		SetHUDRespawnTimer();
		StopAiming();
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Hide attached grenade
	if (AttachedGrenade)
	{
		AttachedGrenade->SetStaticMesh(nullptr);
	}

	// Hide overhead widget
	if (OverheadWidget)
	{
		OverheadWidget->ToggleWithFade(false);
	}

	// Hide crown effect
	if (CrownEffectComponent)
	{
		CrownEffectComponent->DestroyComponent();
	}

	// Spawn elimination bot
	if (EliminationBotEffect)
	{
		const FVector EliminationBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y,
		                                       GetActorLocation().Z + EliminationBotZOffset);
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EliminationBotEffect,
			EliminationBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (EliminationBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, EliminationBotSound, GetActorLocation());
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		this,
		&ABlasterCharacter::RespawnTimerFinished,
		RespawnTime
	);
}

void ABlasterCharacter::HandleCooldown()
{
	EnableLookAroundOnly();

	// Stop respawning if it is in progress
	GetWorldTimerManager().ClearTimer(RespawnTimer);
}

void ABlasterCharacter::UpdateHUDHealth() const
{
	if (PlayerController)
	{
		const float CombinedMaxHealth = Shield > 0.f ? MaxHealth + MaxShield : MaxHealth;
		PlayerController->SetHUDHealth(Health + Shield, CombinedMaxHealth, Health / MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield() const
{
	if (PlayerController)
	{
		PlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo() const
{
	if (PlayerController && CombatComponent && CombatComponent->EquippedWeapon)
	{
		const FLinearColor Color = CombatComponent->EquippedWeapon->GetAmmoColor();

		const int32 Ammo = CombatComponent->EquippedWeapon->GetAmmo();
		PlayerController->SetHUDWeaponAmmo(Ammo, Color);

		const int32 CarriedAmmo = CombatComponent->CarriedAmmo;
		PlayerController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void ABlasterCharacter::UpdateHUDReloadProgress() const
{
	if (PlayerController && CombatComponent && CombatComponent->EquippedWeapon)
	{
		PlayerController->SetHUDReloadProgress(
			CombatComponent->GetReloadProgress(),
			CombatComponent->GetReloadDuration()
		);
	}
}

void ABlasterCharacter::UpdateHUDReloadProgressVisibility(const bool bVisibility) const
{
	if (PlayerController)
	{
		PlayerController->SetHUDReloadProgressVisibility(bVisibility);
	}
}

void ABlasterCharacter::UpdateHUDBuffDurations() const
{
	if (PlayerController && BuffComponent)
	{
		PlayerController->SetHUDBuffDurationBars(
			bIsEliminated ? TArray<FBuffDurationEntry>() : BuffComponent->BuffDurationEntries);
	}
}

void ABlasterCharacter::MulticastGainedMVP_Implementation()
{
	if (CrownEffect && !IsValid(CrownEffectComponent))
	{
		CrownEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownEffect,
			GetRootComponent(),
			Bone::HeadBone,
			FVector(0.f, 0.f, CrownEffectZOffset),
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

	if (CrownEffectComponent)
	{
		CrownEffectComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostMVP_Implementation()
{
	if (CrownEffectComponent)
	{
		CrownEffectComponent->DestroyComponent();
	}
}

void ABlasterCharacter::SpawnShieldBuffEffect(UNiagaraSystem* ShieldBuffEffect)
{
	if (ShieldBuffEffect)
	{
		const bool bShieldEffectAlreadyActive = ShieldBuffEffectComponent && ShieldBuffEffectComponent->IsActive();
		if (!bShieldEffectAlreadyActive)
		{
			ShieldBuffEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				ShieldBuffEffect,
				GetRootComponent(),
				NAME_None,
				FVector(0.f, 0.f, 0.f),
				FRotator::ZeroRotator,
				EAttachLocation::KeepWorldPosition,
				false
			);
		}
	}
	else if (ShieldBuffEffectComponent)
	{
		ShieldBuffEffectComponent->DeactivateImmediate();
	}
}

void ABlasterCharacter::LeaveGame()
{
	if (CombatComponent)
	{
		CombatComponent->DropEquippedWeapon();
		CombatComponent->DropSecondaryWeapon();
		CombatComponent->DropFlag(true);
	}

	MulticastLeaveGame();
}

void ABlasterCharacter::MulticastLeaveGame_Implementation()
{
	if (IsLocallyControlled())
	{
		GetWorldTimerManager().ClearTimer(HUDRespawnTimer);
		OnLeaveGame.Broadcast();
	}
}

void ABlasterCharacter::ServerInitiateLeavingGame_Implementation()
{
	BlasterPlayerState = BlasterPlayerState ? BlasterPlayerState.Get() : GetPlayerState<ABlasterPlayerState>();
	BlasterGameMode = BlasterGameMode ? BlasterGameMode.Get() : GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterPlayerState)
	{
		if (BlasterGameMode)
		{
			BlasterGameMode->PlayerLeavingGame(BlasterPlayerState);
		}
		else
		{
			ALobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
			LobbyGameMode->PlayerLeavingGame(BlasterPlayerState);
		}
	}
}

void ABlasterCharacter::SetTeamColor(const ETeam Team)
{
	if (GetMesh() == nullptr)
	{
		return;
	}

	switch (Team)
	{
	case ETeam::ET_BlueTeam:
		if (BlueMaterial)
		{
			GetMesh()->SetMaterial(0, BlueMaterial);
		}
		DissolveMaterialInstance = BlueDissolveMaterial;
		bTeamColorSet = true;
		break;
	case ETeam::ET_RedTeam:
		if (RedMaterial)
		{
			GetMesh()->SetMaterial(0, RedMaterial);
		}
		DissolveMaterialInstance = RedDissolveMaterial;
		bTeamColorSet = true;
		break;
	case ETeam::ET_NoTeam:
	default: ;
		if (DefaultMaterial)
		{
			GetMesh()->SetMaterial(0, DefaultMaterial);
		}
		DissolveMaterialInstance = DefaultDissolveMaterial;
		break;
	}
}

void ABlasterCharacter::StopAiming()
{
	if (CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->bIsAiming)
	{
		CombatComponent->AimButtonPressed(false);
	}
}

void ABlasterCharacter::StopFiring()
{
	if (CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->bIsFiring)
	{
		CombatComponent->FireButtonPressed(false);
	}
}

TObjectPtr<ABlasterPlayerController> ABlasterCharacter::GetMostRecentDamager() const
{
	if (DamagerTimestamps.IsEmpty())
	{
		return nullptr;
	}

	TArray<TPair<TObjectPtr<AController>, float>> SortedDamagers;
	for (const auto& Pair : DamagerTimestamps)
	{
		SortedDamagers.Add(TPair<TObjectPtr<AController>, float>(Pair.Key, Pair.Value));
	}

	SortedDamagers.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });

	return Cast<ABlasterPlayerController>(SortedDamagers[0].Key);
}

TObjectPtr<ABlasterPlayerController> ABlasterCharacter::GetSecondMostRecentDamager() const
{
	if (DamagerTimestamps.Num() < 2)
	{
		return nullptr;
	}

	TArray<TPair<TObjectPtr<AController>, float>> SortedDamagers;
	for (const auto& Pair : DamagerTimestamps)
	{
		SortedDamagers.Add(TPair<TObjectPtr<AController>, float>(Pair.Key, Pair.Value));
	}

	SortedDamagers.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });

	return Cast<ABlasterPlayerController>(SortedDamagers[1].Key);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeEnhancedInput();

	SetInitialRotation();
	SpawnDefaultWeapon();

	PlayerController = Cast<ABlasterPlayerController>(Controller);
	if (PlayerController)
	{
		PlayerController->PlayerCameraManager->ViewPitchMin = -VIEW_PITCH_LIMIT;
		PlayerController->PlayerCameraManager->ViewPitchMax = VIEW_PITCH_LIMIT;

		const int32 MaxThrowingGrenades = CombatComponent->GetThrowingGrenades();
		PlayerController->SetHUDGrenades(MaxThrowingGrenades);
	}

	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDAmmo();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (IsLocallyControlled())
	{
		DefaultFOV = FollowCamera ? FollowCamera->FieldOfView : DEFAULT_FOV;

		if (GEngine)
		{
			GameUserSettings = Cast<UBlasterGameUserSettings>(GEngine->GetGameUserSettings());
		}

		UpdateHUDReloadProgressVisibility(false);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::PollPlayerStateInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->GetMVP() == BlasterPlayerState)
			{
				MulticastGainedMVP();
			}

			if (HasAuthority())
			{
				CheckIfInsideSpawnZone();
			}
		}
	}
}

void ABlasterCharacter::PollSettingTeamColor()
{
	if (!bTeamColorSet && BlasterPlayerState)
	{
		const ETeam Team = BlasterPlayerState->GetTeam();
		if (Team != ETeam::ET_NoTeam)
		{
			SetTeamColor(Team);
		}
	}
}

void ABlasterCharacter::PollSettingOverheadWidget()
{
	if (!IsLocallyControlled() && BlasterPlayerState && OverheadWidget == nullptr)
	{
		SetOverheadWidget();
	}
}

void ABlasterCharacter::PollSettingDamageWidget()
{
	if (!IsLocallyControlled() && BlasterPlayerState && DamageWidget == nullptr)
	{
		SetDamageWidget();
	}
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	const FVector2d MovementVector = Value.Get<FVector2d>();

	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2d LookAxisVector = Value.Get<FVector2d>();
	const float SensitivityScale = CalculateSensitivityScale();

	AddControllerPitchInput(-LookAxisVector.Y * SensitivityScale);
	AddControllerYawInput(LookAxisVector.X * SensitivityScale);
}

void ABlasterCharacter::CustomJump(const FInputActionValue& Value)
{
	Jump();

	if (HasAuthority())
	{
		const bool bIsFalling = GetCharacterMovement()->IsFalling();
		if (JumpSound && JumpSoundAttenuation && !bIsFalling)
		{
			MulticastPlayJumpSound();
		}
	}
	else
	{
		ServerPlayJumpSound();
	}
}

void ABlasterCharacter::CustomCrouch(const FInputActionValue& Value)
{
	const bool bCrouchPressed = Value.Get<bool>();
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	if (bCrouchPressed && !bIsCrouched && !bIsFalling)
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}

void ABlasterCharacter::Equip(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		ServerEquip();
	}
}

void ABlasterCharacter::Aim(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		const bool bAimPressed = Value.Get<bool>();
		CombatComponent->AimButtonPressed(bAimPressed);
	}
}

void ABlasterCharacter::Fire(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		const bool bFirePressed = Value.Get<bool>();
		CombatComponent->FireButtonPressed(bFirePressed);
	}
}

void ABlasterCharacter::Reload(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->Reload();
	}
}

void ABlasterCharacter::ThrowGrenade(const FInputActionValue& Value)
{
	if (CombatComponent)
	{
		CombatComponent->ThrowGrenade();
	}
}

void ABlasterCharacter::SwitchWeapons(const FInputActionValue& Value)
{
	const bool bCanSwitchWeapons = CombatComponent &&
		CombatComponent->CombatState == ECombatState::ECS_Unoccupied &&
		CombatComponent->EquippedWeapon != nullptr &&
		CombatComponent->SecondaryWeapon != nullptr &&
		!CombatComponent->bIsHoldingFlag;
	if (bCanSwitchWeapons)
	{
		ServerSwitchWeapons();

		if (!HasAuthority())
		{
			CombatComponent->CombatState = ECombatState::ECS_SwitchingWeapons;
			CombatComponent->HandleSwitchWeaponsMontage();
		}
	}
}

void ABlasterCharacter::RotateInPlace(const float DeltaTime)
{
	if (CombatComponent && CombatComponent->bIsHoldingFlag)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		return;
	}

	if (bIsLookAroundOnly)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}

	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > TimeSinceLastMovementReplicationThreshold)
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::AimOffset(const float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	const float Speed = CalculateSpeed();
	const bool bIsFalling = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsFalling) // Standing still, not jumping
	{
		bRotateRootBone = true;
		bUseControllerRotationYaw = true;

		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(
			CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsFalling) // Walking or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;

		bRotateRootBone = false;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	bRotateRootBone = false;

	const float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}

		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::PlayHitReactMontage() const
{
	const bool bCanPlayHitReactMontage = CombatComponent != nullptr && CombatComponent->EquippedWeapon != nullptr &&
		CombatComponent->CombatState == ECombatState::ECS_Unoccupied && !bIsEliminated;
	if (!bCanPlayHitReactMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);

		TArray SectionNames = {
			MontageSection::HitReact1,
			MontageSection::HitReact2
		};
		const int32 RandomIndex = FMath::RandRange(0, SectionNames.Num() - 1);
		const FName SectionName = SectionNames[RandomIndex];

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatorController, AActor* DamageCauser)
{
	BlasterGameMode = BlasterGameMode ? BlasterGameMode.Get() : GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode == nullptr || bIsEliminated)
	{
		return;
	}

	Damage = BlasterGameMode->CalculateDamage(InstigatorController, GetController(), Damage);
	if (Damage <= 0.f)
	{
		return;
	}

	ApplyDamageCauserMultiplier(Damage, DamageCauser);

	ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
	if (AttackerController)
	{
		if (ABlasterCharacter* AttackerCharacter = Cast<ABlasterCharacter>(AttackerController->GetPawn()))
		{
			if (const UBlasterDamageType* BlasterDamageType = Cast<UBlasterDamageType>(DamageType))
			{
				if (AttackerCharacter == this && !BlasterDamageType->IsAoeDamage())
				{
					return; // Skip damage for self-hits with non-AOE damage
				}

				AttackerCharacter->ClientShowDamageNumber(Damage, BlasterDamageType, this);
			}
		}
	}

	CleanupDamagerHistory();

	if (AttackerController && AttackerController != GetController())
	{
		DamagerTimestamps.Add(InstigatorController, GetWorld()->GetTimeSeconds());
	}

	const float DamageToShield = FMath::Min(Damage, Shield);
	const float DamageToHealth = Damage - DamageToShield;

	Shield = FMath::Clamp(Shield - DamageToShield, 0.f, MaxShield);
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Shield == 0.f && BuffComponent)
	{
		BuffComponent->DepleteShield();
	}

	if (Health == 0.f && AttackerController)
	{
		StopReloadMontage();

		PlayerController = PlayerController == nullptr
			                   ? Cast<ABlasterPlayerController>(Controller)
			                   : PlayerController.Get();
		BlasterGameMode->PlayerEliminated(this, PlayerController, AttackerController);
	}
}

void ABlasterCharacter::ClientShowDamageNumber_Implementation(const float DamageAmount,
                                                              const UBlasterDamageType* DamageType,
                                                              ABlasterCharacter* DamagedCharacter)
{
	if (DamagedCharacter && DamagedCharacter->DamageWidget)
	{
		DamagedCharacter->AccumulatedDamage += DamageAmount;

		DamagedCharacter->DamageWidget->SetDamageNumber(DamagedCharacter->AccumulatedDamage, DamageType->IsHeadshot());
		DamagedCharacter->DamageWidget->SetVisibility(ESlateVisibility::Visible);

		DamagedCharacter->GetWorldTimerManager().ClearTimer(DamagedCharacter->DamageAccumulationTimer);
		DamagedCharacter->GetWorldTimerManager().SetTimer(
			DamagedCharacter->DamageAccumulationTimer,
			[DamagedCharacter]
			{
				DamagedCharacter->AccumulatedDamage = 0.f;
				DamagedCharacter->bIsDamageAccumulating = false;

				if (DamagedCharacter && DamagedCharacter->DamageWidget)
				{
					DamagedCharacter->DamageWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			},
			DamagedCharacter->DamageAccumulationTime,
			false
		);

		DamagedCharacter->bIsDamageAccumulating = true;
	}
}

void ABlasterCharacter::SetOverheadWidget()
{
	if (OverheadWidgetComponent == nullptr || IsLocallyControlled())
	{
		return;
	}

	OverheadWidget = Cast<UOverheadWidget>(OverheadWidgetComponent->GetWidget());
	if (OverheadWidget)
	{
		OverheadWidget->ShowPlayerName(this);
	}

	if (const ABlasterCharacter* LocalCharacter = Cast<ABlasterCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (LocalCharacter != this)
		{
			GetWorldTimerManager().SetTimer(
				OverheadWidgetVisibilityTimer,
				this,
				&ABlasterCharacter::UpdateOverheadWidgetVisibility,
				OverheadWidgetVisibilityCheckInterval,
				true
			);
		}
	}
}

void ABlasterCharacter::SetDamageWidget()
{
	if (DamageWidgetComponent == nullptr || IsLocallyControlled())
	{
		return;
	}

	DamageWidget = Cast<UDamageWidget>(DamageWidgetComponent->GetWidget());
}

void ABlasterCharacter::InitializeEnhancedInput() const
{
	if (GEngine == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	if (const ULocalPlayer* FirstGamePlayer = GEngine->GetFirstGamePlayer(GetWorld()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(FirstGamePlayer))
		{
			Subsystem->AddMappingContext(BlasterMappingContext, 0);
		}
	}
}

void ABlasterCharacter::SetInitialRotation()
{
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStartActors);

	const AActor* InitialPlayerStart = nullptr;
	float ClosestDistance = MAX_FLT;

	for (const auto PlayerStartActor : PlayerStartActors)
	{
		const float Distance = FVector::Distance(GetActorLocation(), PlayerStartActor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			InitialPlayerStart = PlayerStartActor;
		}
	}

	if (Controller)
	{
		Controller->SetControlRotation(InitialPlayerStart->GetActorRotation());
	}

	StartingAimRotation = FRotator(0.f, InitialPlayerStart->GetActorRotation().Yaw, 0.f);
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode ? BlasterGameMode.Get() : GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && DefaultWeaponClass && !bIsEliminated)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);

		if (CombatComponent)
		{
			CombatComponent->EquipWeapon(DefaultWeapon);
		}
	}
}

void ABlasterCharacter::SetShieldEffectPosition() const
{
	if (ShieldBuffEffectComponent)
	{
		const FVector PelvisLocation = GetMesh()->GetSocketLocation(Bone::PelvisBone);
		ShieldBuffEffectComponent->SetWorldLocation(PelvisLocation);
		ShieldBuffEffectComponent->SetWorldRotation(FRotator::ZeroRotator);
	}
}

void ABlasterCharacter::SetCrownEffectPosition() const
{
	if (CrownEffectComponent)
	{
		const FVector HeadLocation = GetMesh()->GetSocketLocation(Bone::HeadBone);
		const FVector Offset(0.f, 0.f, CrownEffectZOffset);
		CrownEffectComponent->SetWorldLocation(HeadLocation + Offset);
		CrownEffectComponent->SetWorldRotation(FRotator::ZeroRotator);
	}
}

void ABlasterCharacter::SetOverheadWidgetPosition() const
{
	if (OverheadWidgetComponent)
	{
		const FVector RootLocation = GetMesh()->GetSocketLocation(Bone::RootBone);
		const FVector Offset(0.f, 0.f, OverheadWidgetZOffset);
		OverheadWidgetComponent->SetWorldLocation(RootLocation + Offset);
	}
}

void ABlasterCharacter::SetDamageWidgetPosition() const
{
	if (DamageWidgetComponent)
	{
		const FVector RootLocation = GetMesh()->GetSocketLocation(Bone::RootBone);
		const FVector Offset(0.f, 0.f, DamageWidgetZOffset);
		DamageWidgetComponent->SetWorldLocation(RootLocation + Offset);
	}
}

void ABlasterCharacter::OnRep_WeaponToPickup(AWeapon* PreviousWeaponToPickup) const
{
	if (WeaponToPickup)
	{
		WeaponToPickup->ShowPickupWidget(true);
	}

	if (PreviousWeaponToPickup)
	{
		PreviousWeaponToPickup->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerEquip_Implementation()
{
	if (CombatComponent)
	{
		if (IsHoldingFlag())
		{
			CombatComponent->DropFlag(false);
		}
		else
		{
			CombatComponent->EquipWeapon(WeaponToPickup);
		}
	}
}

void ABlasterCharacter::ServerSwitchWeapons_Implementation()
{
	if (CombatComponent)
	{
		CombatComponent->SwitchWeapons();
	}
}

float ABlasterCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map pitch from [270, 360) to [-90, 0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::TurnInPlace(const float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, YAW_INTERP_SPEED);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraTooClose() const
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const bool bCameraTooClose = (FollowCamera->GetComponentLocation() - GetActorLocation()).Size() <
		CameraThreshold;

	if (CombatComponent)
	{
		if (CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bCameraTooClose;
			CombatComponent->EquippedWeapon->SetDamageBuffEffectVisibility(!bCameraTooClose);
		}

		if (CombatComponent->SecondaryWeapon && CombatComponent->SecondaryWeapon->GetWeaponMesh())
		{
			CombatComponent->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = bCameraTooClose;
		}

		if (CombatComponent->Flag && CombatComponent->Flag->GetFlagMesh())
		{
			CombatComponent->Flag->GetFlagMesh()->bOwnerNoSee = bCameraTooClose;
		}
	}

	if (ShieldBuffEffectComponent)
	{
		ShieldBuffEffectComponent->SetVisibility(!bCameraTooClose);
	}

	if (CrownEffectComponent)
	{
		CrownEffectComponent->SetVisibility(!bCameraTooClose);
	}

	GetMesh()->SetVisibility(!bCameraTooClose);
}

void ABlasterCharacter::OnRep_Health(const float LastHealth)
{
	UpdateHUDHealth();

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::OnRep_Shield(const float LastShield)
{
	UpdateHUDShield();
	UpdateHUDHealth();

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::PlayEliminationMontage() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EliminationMontage)
	{
		AnimInstance->Montage_Play(EliminationMontage);
	}
}

void ABlasterCharacter::RespawnTimerFinished()
{
	BlasterGameMode = BlasterGameMode ? BlasterGameMode.Get() : GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, GetController());
	}
}

void ABlasterCharacter::SetHUDRespawnTimer()
{
	if (PlayerController == nullptr)
	{
		return;
	}

	PlayerController->SetHUDRespawnBoxVisibility(true);
	PlayerController->SetHUDRespawnTime(RespawnTime);

	const float StartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(
		HUDRespawnTimer,
		[this, StartTime]
		{
			const float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;
			const float RespawnTimeLeft = RespawnTime - ElapsedTime;
			PlayerController->SetHUDRespawnTime(RespawnTimeLeft);

			if (RespawnTimeLeft <= 0)
			{
				GetWorldTimerManager().ClearTimer(HUDRespawnTimer);
				PlayerController->SetHUDRespawnBoxVisibility(false);
			}
		},
		1.f,
		true
	);
}

void ABlasterCharacter::UpdateDissolveMaterial(const float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(MaterialParameter::Dissolve, DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);

	if (DissolveTimeline && DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::EnableLookAroundOnly()
{
	bIsLookAroundOnly = true;

	if (CombatComponent)
	{
		CombatComponent->FireButtonPressed(false);
	}

	if (const ULocalPlayer* FirstGamePlayer = GEngine->GetFirstGamePlayer(GetWorld()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(FirstGamePlayer))
		{
			Subsystem->RemoveMappingContext(BlasterMappingContext);
			Subsystem->AddMappingContext(LookAroundMappingContext, 0);
		}
	}
}

void ABlasterCharacter::ApplyDamageCauserMultiplier(float& Damage, const AActor* DamageCauser) const
{
	if (const ABlasterCharacter* DamageCharacter = Cast<ABlasterCharacter>(DamageCauser->GetOwner()))
	{
		if (DamageCharacter->CombatComponent)
		{
			Damage *= DamageCharacter->CombatComponent->GetDamageMultiplier();
		}
	}
}

void ABlasterCharacter::CheckIfInsideSpawnZone()
{
	TArray<AActor*> SpawnZoneActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnZone::StaticClass(), SpawnZoneActors);

	for (AActor* SpawnZoneActor : SpawnZoneActors)
	{
		if (const ASpawnZone* SpawnZone = Cast<ASpawnZone>(SpawnZoneActor))
		{
			if (SpawnZone->GetTeam() == GetTeam() && SpawnZone->GetOverlapBox()->IsOverlappingActor(this))
			{
				SetIsInsideSpawnZone(true);
				return;
			}
		}
	}
}

float ABlasterCharacter::CalculateSensitivityScale() const
{
	if (FollowCamera == nullptr)
	{
		return 1.f;
	}

	const float SensitivityMultiplier = GameUserSettings ? GameUserSettings->GetMouseSensitivity() : 1.f;

	const float CurrentFOV = FollowCamera->FieldOfView;

	// Convert to radians for tangent calculation
	const float CurrentFOVRadians = FMath::DegreesToRadians(CurrentFOV);
	const float DefaultFOVRadians = FMath::DegreesToRadians(DefaultFOV);

	// Calculate and return the sensitivity scale using tangent method
	const float FOVScaleFactor = FMath::Tan(CurrentFOVRadians * 0.5f) / FMath::Tan(DefaultFOVRadians * 0.5f);

	return BASE_MOUSE_SENSITIVITY * SensitivityMultiplier * FOVScaleFactor;
}

ABlasterCharacter* ABlasterCharacter::GetLocallyControlledCharacter() const
{
	if (IsValid(LocallyControlledCharacter))
	{
		return LocallyControlledCharacter;
	}

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			return Cast<ABlasterCharacter>(PC->GetPawn());
		}
	}

	return nullptr;
}

void ABlasterCharacter::UpdateOverheadWidgetDistance()
{
	if (OverheadWidget == nullptr || bIsEliminated || IsLocallyControlled())
	{
		return;
	}

	LocallyControlledCharacter = GetLocallyControlledCharacter();
	if (LocallyControlledCharacter == nullptr)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Distance(
		LocallyControlledCharacter->GetActorLocation(),
		GetActorLocation()
	);

	bIsWithinOverheadWidgetVisibilityDistance = DistanceToPlayer <= OverheadWidgetVisibilityDistance;

	// Adjust Z offset based on distance
	OverheadWidgetZOffset = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, OverheadWidgetVisibilityDistance),
		FVector2D(OverheadWidgetZOffsetClose, OverheadWidgetZOffsetFar),
		DistanceToPlayer
	);

	// Adjust scale based on distance
	if (WidgetScaleCurve)
	{
		const float NormalizedDistance = FMath::Clamp(DistanceToPlayer / OverheadWidgetVisibilityDistance, 0.f, 1.f);
		const float CurveValue = WidgetScaleCurve->GetFloatValue(NormalizedDistance);
		const float Scale = FMath::Lerp(OverheadWidgetScaleFar, OverheadWidgetScaleClose, CurveValue);

		OverheadWidget->SetRenderScale(FVector2D(Scale, Scale));
	}

	// Adjust opacity based on distance
	const float Opacity = FMath::GetMappedRangeValueClamped(
		FVector2D(OverheadWidgetVisibilityDistance / 2.f, OverheadWidgetVisibilityDistance),
		FVector2D(OverheadWidgetOpacityClose, OverheadWidgetOpacityFar),
		DistanceToPlayer
	);

	OverheadWidget->SetRenderOpacity(Opacity);
}

void ABlasterCharacter::UpdateOverheadWidgetVisibility()
{
	if (OverheadWidget == nullptr || bIsEliminated)
	{
		return;
	}

	LocallyControlledCharacter = GetLocallyControlledCharacter();
	if (LocallyControlledCharacter == nullptr)
	{
		return;
	}

	if (!bIsWithinOverheadWidgetVisibilityDistance)
	{
		OverheadWidget->ToggleWithFade(false);
		return;
	}

	const FVector CharacterCenter = GetActorLocation();

	if (GetCapsuleComponent() == nullptr)
	{
		return;
	}

	const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * CAPSULE_VISIBILITY_SCALE;
	const float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * CAPSULE_VISIBILITY_SCALE;

	const TArray TraceCheckPoints = {
		CharacterCenter, // Center
		CharacterCenter + FVector(0.f, 0.f, CapsuleHalfHeight), // Head
		CharacterCenter + FVector(CapsuleRadius, 0.f, 0.f), // Right
		CharacterCenter + FVector(-CapsuleRadius, 0.f, 0.f), // Left
		CharacterCenter + FVector(0.f, CapsuleRadius, 0.f), // Front
		CharacterCenter + FVector(0.f, -CapsuleRadius, 0.f) // Back
	};

	const FVector StartLocation = LocallyControlledCharacter->GetFollowCamera()->GetComponentLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(LocallyControlledCharacter);

	bool bIsFullyVisible = true;

	for (const FVector& EndLocation : TraceCheckPoints)
	{
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			ECC_Visibility,
			QueryParams
		);

		if (HitResult.bBlockingHit)
		{
			bIsFullyVisible = false;
			break;
		}
	}

	OverheadWidget->ToggleWithFade(bIsFullyVisible);
}

void ABlasterCharacter::UpdateDamageWidgetDistance()
{
	if (DamageWidget == nullptr || bIsEliminated || IsLocallyControlled())
	{
		return;
	}

	LocallyControlledCharacter = GetLocallyControlledCharacter();
	if (LocallyControlledCharacter == nullptr)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Distance(
		LocallyControlledCharacter->GetActorLocation(),
		GetActorLocation()
	);

	// Adjust Z offset based on distance
	DamageWidgetZOffset = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, OverheadWidgetVisibilityDistance),
		FVector2D(DamageWidgetZOffsetClose, DamageWidgetZOffsetFar),
		DistanceToPlayer
	);

	if (WidgetScaleCurve)
	{
		const float NormalizedDistance = FMath::Clamp(DistanceToPlayer / OverheadWidgetVisibilityDistance, 0.f, 1.f);
		const float CurveValue = WidgetScaleCurve->GetFloatValue(NormalizedDistance);
		const float Scale = FMath::Lerp(DamageWidgetScaleFar, DamageWidgetScaleClose, CurveValue);

		DamageWidget->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void ABlasterCharacter::CleanupDamagerHistory()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	TArray<TObjectPtr<AController>> ControllersToRemove;

	for (const auto& Pair : DamagerTimestamps)
	{
		if (CurrentTime - Pair.Value > DamagerTrackingTime)
		{
			ControllersToRemove.Add(Pair.Key);
		}
	}

	for (const auto& DamagerController : ControllersToRemove)
	{
		DamagerTimestamps.Remove(DamagerController);
	}
}

void ABlasterCharacter::MulticastPlayJumpSound_Implementation()
{
	if (JumpSound && JumpSoundAttenuation)
	{
		UGameplayStatics::SpawnSoundAttached(
			JumpSound,
			GetRootComponent(),
			NAME_None,
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			JumpSoundAttenuation,
			nullptr,
			false
		);
	}
}

void ABlasterCharacter::ServerPlayJumpSound_Implementation()
{
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	if (JumpSound && JumpSoundAttenuation && !bIsFalling)
	{
		MulticastPlayJumpSound();
	}
}
