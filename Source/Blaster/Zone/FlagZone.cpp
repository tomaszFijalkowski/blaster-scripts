// Fill out your copyright notice in the Description page of Project Settings.

#include "FlagZone.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/GameMode/CaptureTheFlagGameMode.h"
#include "Blaster/Weapon/Flag.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	SetRootComponent(OverlapSphere);

	SetCanBeDamaged(false);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapSphere && HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereBeginOverlap);
	}
}

void AFlagZone::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                     const FHitResult& SweepResult)
{
	AFlag* OverlappingFlag = Cast<AFlag>(OtherActor);
	if (OverlappingFlag && OverlappingFlag->GetOwner() && OverlappingFlag->GetTeam() != Team)
	{
		if (ACaptureTheFlagGameMode* GameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>())
		{
			GameMode->FlagCaptured(OverlappingFlag, this);

			MulticastPlayFlagCaptureEffects(OverlappingFlag->GetActorTransform());

			OverlappingFlag->ResetFlag();
		}
	}
}

void AFlagZone::MulticastPlayFlagCaptureEffects_Implementation(const FTransform& FlagTransform)
{
	if (FlagCaptureEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			FlagCaptureEffect,
			FlagTransform.GetLocation(),
			FlagTransform.GetRotation().Rotator()
		);
	}

	if (FlagCaptureSound)
	{
		UGameplayStatics::SpawnSound2D(this, FlagCaptureSound);
	}
}
