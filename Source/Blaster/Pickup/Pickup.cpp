// Fill out your copyright notice in the Description page of Project Settings.

#include "Pickup.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->AddLocalOffset(FVector(0.f, 0.f, 100.f));
	OverlapSphere->SetSphereRadius(32.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(OverlapSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);

	SetCanBeDamaged(false);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapSphere && HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			CheckForOverlappingActorsTimer,
			this,
			&APickup::CheckForOverlappingActors,
			CheckForOverlappingActorsInterval,
			true,
			CheckForOverlappingActorsInterval
		);
	}
}

void APickup::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	if (bIsBeingDestroyed)
	{
		return;
	}

	OverlappedCharacter = Cast<ABlasterCharacter>(OtherActor);
}

void APickup::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleHovering(DeltaTime);
}

void APickup::DeferredDestroy()
{
	bIsBeingDestroyed = true;

	if (OverlappedCharacter)
	{
		MulticastHandleDestroy(OverlappedCharacter->GetRootComponent());
	}

	GetWorldTimerManager().ClearTimer(CheckForOverlappingActorsTimer);
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		[this] { Destroy(); },
		DestroyTime,
		false
	);
}

void APickup::MulticastHandleDestroy_Implementation(USceneComponent* ComponentToAttach)
{
	HidePickup();
	SpawnPickupEffect();
	PlayPickupSound(ComponentToAttach);
}

void APickup::CheckForOverlappingActors()
{
	check(OverlapSphere);

	if (!OverlapSphere->OnComponentBeginOverlap.IsAlreadyBound(this, &APickup::OnSphereBeginOverlap))
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APickup::OnSphereBeginOverlap);
	}

	TArray<AActor*> OverlappingActors;
	OverlapSphere->GetOverlappingActors(OverlappingActors, ABlasterCharacter::StaticClass());

	const FVector Location = OverlapSphere->GetComponentLocation();
	OverlappingActors.Sort([Location](const AActor& A, const AActor& B)
	{
		return FVector::Dist(Location, A.GetActorLocation()) < FVector::Dist(Location, B.GetActorLocation());
	});

	for (AActor* Actor : OverlappingActors)
	{
		if (ABlasterCharacter* OverlappingCharacter = Cast<ABlasterCharacter>(Actor))
		{
			OnSphereBeginOverlap(OverlapSphere, OverlappingCharacter, nullptr, 0, false, FHitResult());
		}
	}
}

void APickup::HandleHovering(const float DeltaTime)
{
	RunningTime += DeltaTime;

	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f, BaseTurnRate * DeltaTime, 0.f));

		const double TransformedSin = FMath::Sin(RunningTime * TimeConstant) * Amplitude;
		PickupMesh->AddWorldOffset(FVector(0.f, 0.f, TransformedSin));
	}
}

void APickup::HidePickup() const
{
	if (OverlapSphere)
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (RootComponent)
	{
		TArray<USceneComponent*> Components;
		RootComponent->GetChildrenComponents(true, Components);
		for (USceneComponent* Component : Components)
		{
			Component->SetVisibility(false);

			if (UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(Component))
			{
				NiagaraComponent->Deactivate();
			}
		}
	}
}

void APickup::SpawnPickupEffect() const
{
	if (PickupEffect)
	{
		const FVector OffsetVector(0.f, 0.f, PickupEffectOffsetZ);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			PickupEffectComponent->GetComponentLocation() + OffsetVector,
			PickupEffectComponent->GetComponentRotation()
		);
	}
}

void APickup::PlayPickupSound(USceneComponent* ComponentToAttach) const
{
	if (PickupSound && PickupSoundAttenuation && ComponentToAttach)
	{
		UGameplayStatics::SpawnSoundAttached(
			PickupSound,
			ComponentToAttach,
			NAME_None,
			ComponentToAttach->GetComponentLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			PickupSoundAttenuation,
			nullptr,
			false
		);
	}
}
