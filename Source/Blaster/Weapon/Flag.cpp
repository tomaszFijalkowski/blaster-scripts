// Fill out your copyright notice in the Description page of Project Settings.

#include "Flag.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/HUD/FlagResetWidget.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "Components/TimelineComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SetRootComponent(FlagMesh);

	GetOverlapSphere()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);

	DropTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DropTimeline"));

	FlagResetWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("FlagResetWidget"));
	FlagResetWidgetComponent->SetupAttachment(RootComponent);
	FlagResetWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	FlagResetWidgetComponent->SetDrawAtDesiredSize(true);
}

void AFlag::Tick(const float DeltaTime)
{
	// Avoid calling Super::Tick() to prevent the flag from hovering

	if (WeaponState == EWeaponState::EWS_Dropped)
	{
		UpdateFlagResetWidgetDistance();
		SetFlagResetWidgetComponentPosition();
	}
}

void AFlag::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFlag, FlagResetServerStartTime);
}

void AFlag::Dropped()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->SetHoldingFlag(false);
	}

	SetWeaponState(EWeaponState::EWS_Dropped);

	const FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);

	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

void AFlag::ShowPickupWidget(const bool bShowWidget)
{
	Super::ShowPickupWidget(bShowWidget);

	if (GetWorldTimerManager().IsTimerActive(UpdateFlagResetWidgetTimer))
	{
		ShowFlagResetWidget(!bShowWidget);
	}
}

FVector AFlag::GetPickupSocketLocation() const
{
	if (FlagMesh)
	{
		return FlagMesh->GetSocketLocation(Socket::PickupSocket);
	}

	return FVector::ZeroVector;
}

bool AFlag::CanBeDropped(const bool bPerformDropLineTrace)
{
	if (bPerformDropLineTrace)
	{
		InitialDropLocation = GetActorLocation();
		InitialDropRotation = GetActorRotation();
	}

	const FVector TargetDropLocationToCheck = bPerformDropLineTrace ? GetDropLineTrace() : TargetDropLocation;
	const float DropDistance = FVector::Dist(InitialDropLocation, TargetDropLocationToCheck);

	if (DropDistance > MAX_DROP_DISTANCE)
	{
		return false;
	}

	// Perform a sphere trace to check for collisions
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		GetActorLocation(),
		GetActorLocation(),
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(MIN_CLEARANCE_RADIUS),
		QueryParams
	);

	return !bHit;
}

void AFlag::ResetFlag()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->SetHoldingFlag(false);

		if (UCombatComponent* CombatComponent = OwnerCharacter->GetCombatComponent())
		{
			CombatComponent->RecalculateMovementSpeed();
		}
	}

	MulticastResetFlag();
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();

	if (DropCurve)
	{
		FOnTimelineFloat InterpFunction{};
		FOnTimelineEvent TimelineFinished{};

		const FName InterpFunctionName = GET_FUNCTION_NAME_CHECKED(AFlag, HandleDropProgress);
		InterpFunction.BindUFunction(this, InterpFunctionName);

		const FName TimelineFinishedFunctionName = GET_FUNCTION_NAME_CHECKED(AFlag, OnDropFinished);
		TimelineFinished.BindUFunction(this, TimelineFinishedFunctionName);

		DropTimeline->AddInterpFloat(DropCurve, InterpFunction, FName("Alpha"));
		DropTimeline->SetTimelineFinishedFunc(TimelineFinished);
		DropTimeline->SetPlayRate(DropPlayRate);
	}

	InitialTransform = GetActorTransform();

	SetFlagResetWidget();
}

void AFlag::HandleWeaponInitialState()
{
	// Avoid calling Super::HandleWeaponInitialState() to prevent the flag from hovering

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AFlag::HandleWeaponEquippedState()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(FlagResetTimer);
	}

	GetWorldTimerManager().ClearTimer(UpdateFlagResetWidgetTimer);

	ShowFlagResetWidget(false);
	ShowPickupWidget(false);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FlagMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	DropTimeline->Stop();
}

void AFlag::HandleWeaponDroppedState()
{
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (HasAuthority())
	{
		InitialDropLocation = GetActorLocation();
		InitialDropRotation = GetActorRotation();

		TargetDropLocation = GetDropLineTrace();
		TargetDropRotation = FRotator(0, InitialDropRotation.Yaw, 0);

		if (CanBeDropped(false))
		{
			MulticastHandleWeaponDroppedState(InitialDropLocation, InitialDropRotation,
			                                  TargetDropLocation, TargetDropRotation);
			bIsFalling = true;
			DropTimeline->PlayFromStart();
		}
		else
		{
			ResetFlag();
		}
	}
}

void AFlag::MulticastHandleWeaponDroppedState_Implementation(const FVector AuthInitialDropLocation,
                                                             const FRotator AuthInitialDropRotation,
                                                             const FVector AuthTargetDropLocation,
                                                             const FRotator AuthTargetDropRotation)
{
	if (!HasAuthority())
	{
		InitialDropLocation = AuthInitialDropLocation;
		InitialDropRotation = AuthInitialDropRotation;
		TargetDropLocation = AuthTargetDropLocation;
		TargetDropRotation = AuthTargetDropRotation;
		DropTimeline->PlayFromStart();
	}
}

void AFlag::MulticastResetFlag_Implementation()
{
	GetWorldTimerManager().ClearTimer(UpdateFlagResetWidgetTimer);

	if (FlagResetSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, FlagResetSound, GetActorLocation());
	}

	ShowFlagResetWidget(false);
	SetActorTransform(InitialTransform);
	SetWeaponState(EWeaponState::EWS_Initial);

	const FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);

	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(FlagResetTimer);

		// if (OwnerCharacter)
		// {
		// 	if (UCombatComponent* CombatComponent = OwnerCharacter->GetCombatComponent())
		// 	{
		// 		CombatComponent->RecalculateMovementSpeed(); // Handled in OnRep method for clients
		// 	}
		// }

		SetOwner(nullptr);
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
}

void AFlag::HandleDropProgress(const float Value)
{
	const FVector NewLocation = FMath::Lerp(InitialDropLocation, TargetDropLocation, Value);
	const FRotator NewRotation = FMath::Lerp(InitialDropRotation, TargetDropRotation, Value);

	FlagMesh->SetWorldLocation(NewLocation);
	FlagMesh->SetWorldRotation(NewRotation);
}

void AFlag::OnDropFinished()
{
	ShowFlagResetWidget(true);

	if (HasAuthority())
	{
		bIsFalling = false;

		// Use server's game state time for synchronization
		if (const AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			FlagResetServerStartTime = GameState->GetServerWorldTimeSeconds();
		}

		GetWorldTimerManager().SetTimer(
			FlagResetTimer,
			this,
			&AFlag::ResetFlag,
			FlagResetTime,
			false
		);
	}

	if (FlagResetWidget)
	{
		FlagResetWidget->SetFlagResetRemainingTime(FlagResetTime);
	}

	GetWorldTimerManager().SetTimer(
		UpdateFlagResetWidgetTimer,
		this,
		&AFlag::UpdateFlagResetRemainingTime,
		0.1f,
		true
	);
}

FVector AFlag::GetDropLineTrace()
{
	const FVector StartLocation = GetActorLocation();
	const FVector EndLocation = StartLocation - FVector(0, 0, TRACE_LENGTH);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			QueryParams.AddIgnoredComponent(PrimitiveComponent);
		}
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	return bHit
		       ? HitResult.Location + FVector(0, 0, DropOffset)
		       : EndLocation + FVector(0, 0, DropOffset);
}

void AFlag::SetFlagResetWidget()
{
	if (FlagResetWidgetComponent)
	{
		FlagResetWidget = Cast<UFlagResetWidget>(FlagResetWidgetComponent->GetWidget());

		if (FlagResetWidget)
		{
			FlagResetWidget->SetFlagResetImage(Team);
		}
	}
}

void AFlag::SetFlagResetWidgetComponentPosition()
{
	if (FlagResetWidgetComponent)
	{
		const FVector SocketLocation = GetPickupSocketLocation();
		const FVector Offset(0.f, 0.f, FlagResetWidgetZOffset);
		FlagResetWidgetComponent->SetWorldLocation(SocketLocation + Offset);
	}
}

void AFlag::ShowFlagResetWidget(const bool bShowWidget)
{
	if (FlagResetWidget)
	{
		FlagResetWidget->SetVisibility(bShowWidget ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void AFlag::UpdateFlagResetWidgetDistance()
{
	if (FlagResetWidget == nullptr)
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

	bIsWithinFlagResetWidgetVisibilityDistance = DistanceToPlayer <= FlagResetWidgetVisibilityDistance;

	// Adjust Z offset based on distance
	FlagResetWidgetZOffset = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, FlagResetWidgetVisibilityDistance),
		FVector2D(FlagResetWidgetZOffsetClose, FlagResetWidgetZOffsetFar),
		DistanceToPlayer
	);

	if (WidgetScaleCurve)
	{
		const float NormalizedDistance =
			FMath::Clamp(DistanceToPlayer / FlagResetWidgetVisibilityDistance, 0.f, 1.f);
		const float CurveValue = WidgetScaleCurve->GetFloatValue(NormalizedDistance);

		// Adjust scale based on distance
		const float Scale = FMath::Lerp(FlagResetWidgetScaleFar, FlagResetWidgetScaleClose, CurveValue);
		FlagResetWidget->SetRenderScale(FVector2D(Scale, Scale));

		// Adjust translation Y based on distance
		const float TranslationY = FMath::Lerp(0.f, FlagResetWidgetTranslationYFar, CurveValue);
		FlagResetWidget->SetRenderTranslation(FVector2D(0.f, TranslationY));
	}
}

void AFlag::UpdateFlagResetRemainingTime()
{
	if (FlagResetWidget && FlagResetServerStartTime > 0.f)
	{
		if (const AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			// Use game state's synchronized time
			const float CurrentServerTime = GameState->GetServerWorldTimeSeconds();
			const float ElapsedTime = CurrentServerTime - FlagResetServerStartTime;
			float RemainingTime = FlagResetTime - ElapsedTime;
			RemainingTime = FMath::Max(0.f, RemainingTime);

			FlagResetWidget->SetFlagResetRemainingTime(RemainingTime);
		}
	}
}

ABlasterCharacter* AFlag::GetLocallyControlledCharacter() const
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
