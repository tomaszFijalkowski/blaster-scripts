// Fill out your copyright notice in the Description page of Project Settings.

#include "SpawnZone.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"

ASpawnZone::ASpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	SetRootComponent(OverlapBox);

	SetCanBeDamaged(false);
}

void ASpawnZone::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapBox && HasAuthority())
	{
		OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &ASpawnZone::OnBoxBeginOverlap);
		OverlapBox->OnComponentEndOverlap.AddDynamic(this, &ASpawnZone::OnBoxEndOverlap);
	}
}

void ASpawnZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (OverlappingCharacter->GetTeam() == Team && !OverlappingCharacter->IsInsideSpawnZone())
		{
			OverlappingCharacter->SetIsInsideSpawnZone(true);
		}
	}
}

void ASpawnZone::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (OverlappingCharacter->GetTeam() == Team && OverlappingCharacter->IsInsideSpawnZone())
		{
			OverlappingCharacter->SetIsInsideSpawnZone(false);
		}
	}
}
