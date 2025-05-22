// Fill out your copyright notice in the Description page of Project Settings.

#include "ThrowingGrenadePickup.h"
#include "Blaster/Character/BlasterCharacter.h"

AThrowingGrenadePickup::AThrowingGrenadePickup()
{
	PickupMesh->SetRenderCustomDepth(true);
}

void AThrowingGrenadePickup::BeginPlay()
{
	if (PickupMesh)
	{
		PickupMesh->MarkRenderStateDirty();
		PickupMesh->SetCustomDepthStencilValue(ThrowingGrenadeCustomDepthStencilValue);
	}

	Super::BeginPlay();
}

void AThrowingGrenadePickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                                  const FHitResult& SweepResult)
{
	if (bIsBeingDestroyed)
	{
		return;
	}

	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		UCombatComponent* CombatComponent = BlasterCharacter->GetCombatComponent();
		if (CombatComponent && !CombatComponent->HasMaxThrowingGrenades())
		{
			CombatComponent->PickupThrowingGrenades(Amount);

			DeferredDestroy();
		}
	}
}
