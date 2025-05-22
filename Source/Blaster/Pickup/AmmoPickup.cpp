// Fill out your copyright notice in the Description page of Project Settings.

#include "AmmoPickup.h"
#include "Blaster/Character/BlasterCharacter.h"

AAmmoPickup::AAmmoPickup()
{
	PickupMesh->SetRenderCustomDepth(true);
}

void AAmmoPickup::BeginPlay()
{
	if (PickupMesh)
	{
		PickupMesh->MarkRenderStateDirty();
		PickupMesh->SetCustomDepthStencilValue(AmmoCustomDepthStencilValue);
	}

	Super::BeginPlay();
}

void AAmmoPickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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
		if (CombatComponent && !CombatComponent->IsCarriedAmmoFull(WeaponType))
		{
			CombatComponent->PickupAmmo(WeaponType, AmmoAmount);

			DeferredDestroy();
		}
	}
}
