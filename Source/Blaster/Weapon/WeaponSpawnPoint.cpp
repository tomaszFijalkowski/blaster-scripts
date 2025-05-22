// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponSpawnPoint.h"
#include "Weapon.h"

AWeaponSpawnPoint::AWeaponSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetHiddenInGame(true);

	WeaponMesh->SetRenderCustomDepth(true);
}

#if WITH_EDITOR
void AWeaponSpawnPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property != nullptr
		                           ? PropertyChangedEvent.Property->GetFName()
		                           : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AWeaponSpawnPoint, WeaponClass))
	{
		if (WeaponClass)
		{
			const int32 CustomDepthStencilValue =
				WeaponClass->GetDefaultObject<AWeapon>()->GetWeaponCustomDepthStencilValue();
			WeaponMesh->SetCustomDepthStencilValue(CustomDepthStencilValue);
			WeaponMesh->MarkRenderStateDirty();
		}
	}
}
#endif

void AWeaponSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	SpawnWeapon();
}

void AWeaponSpawnPoint::StartSpawnWeaponTimer()
{
	if (SpawnedWeapon && SpawnedWeapon->OnPickedUp.IsBound())
	{
		SpawnedWeapon->OnPickedUp.RemoveDynamic(this, &AWeaponSpawnPoint::StartSpawnWeaponTimer);
	}

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			SpawnWeaponTimer,
			this,
			&AWeaponSpawnPoint::SpawnWeapon,
			SpawnWeaponTime,
			false
		);
	}
}

void AWeaponSpawnPoint::SpawnWeapon()
{
	if (WeaponClass && WeaponMesh && HasAuthority())
	{
		SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, WeaponMesh->GetComponentTransform());

		if (SpawnedWeapon)
		{
			SpawnedWeapon->OnPickedUp.AddDynamic(this, &AWeaponSpawnPoint::StartSpawnWeaponTimer);
		}
	}
}
