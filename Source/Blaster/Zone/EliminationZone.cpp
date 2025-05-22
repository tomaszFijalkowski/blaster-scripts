// Fill out your copyright notice in the Description page of Project Settings.

#include "EliminationZone.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/AoeDamageType.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AEliminationZone::AEliminationZone()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	SetRootComponent(OverlapBox);

	SetCanBeDamaged(false);
}

void AEliminationZone::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapBox && HasAuthority())
	{
		OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &AEliminationZone::OnBoxBeginOverlap);
	}
}

void AEliminationZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                         const FHitResult& SweepResult)
{
	if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		const float Damage = OverlappingCharacter->GetMaxHealth() + OverlappingCharacter->GetMaxShield();
		UGameplayStatics::ApplyDamage(
			OverlappingCharacter,
			Damage,
			OverlappingCharacter->GetController(),
			this,
			UAoeDamageType::StaticClass()
		);
	}
}
