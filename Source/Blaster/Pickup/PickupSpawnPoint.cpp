// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupSpawnPoint.h"
#include "NiagaraComponent.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PickupPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupPreviewMesh"));
	PickupPreviewMesh->SetupAttachment(RootComponent);
	PickupPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupPreviewMesh->SetHiddenInGame(true);

	PickupPreviewEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupPreviewEffectComponent"));
	PickupPreviewEffectComponent->SetupAttachment(RootComponent);
	PickupPreviewEffectComponent->SetHiddenInGame(true);
}

void APickupSpawnPoint::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	SpawnPickupTimerFinished();
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnPickupTime,
		false
	);
}

void APickupSpawnPoint::SpawnPickup()
{
	if (PickupClass)
	{
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClass, GetActorTransform());

		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}
