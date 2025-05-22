// Fill out your copyright notice in the Description page of Project Settings.

#include "TeleportZone.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/TargetPoint.h"

ATeleportZone::ATeleportZone()
{
	PrimaryActorTick.bCanEverTick = false;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	SetRootComponent(OverlapBox);

	SetCanBeDamaged(false);
}

void ATeleportZone::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapBox && HasAuthority())
	{
		OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &ATeleportZone::OnBoxBeginOverlap);
	}
}

void ATeleportZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                      const FHitResult& SweepResult)
{
	if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		OverlappingCharacter->SetActorLocation(Target->GetActorLocation());
	}
}
