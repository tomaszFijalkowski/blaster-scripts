// Fill out your copyright notice in the Description page of Project Settings.

#include "ClearZone.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/BoxComponent.h"

AClearZone::AClearZone()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	SetRootComponent(OverlapBox);

	SetCanBeDamaged(false);
}

void AClearZone::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapBox && HasAuthority())
	{
		OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &AClearZone::OnBoxBeginOverlap);
	}
}

void AClearZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	if (AWeapon* OverlappingWeapon = Cast<AWeapon>(OtherActor))
	{
		if (OverlappingWeapon->GetWeaponType() != EWeaponType::EWT_Flag)
		{
			OverlappingWeapon->Destroy();
		}
	}
}
