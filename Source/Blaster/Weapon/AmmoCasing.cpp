// Fill out your copyright notice in the Description page of Project Settings.

#include "AmmoCasing.h"
#include "Kismet/GameplayStatics.h"

AAmmoCasing::AAmmoCasing()
{
	PrimaryActorTick.bCanEverTick = false;

	AmmoCasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoCasingMesh"));
	SetRootComponent(AmmoCasingMesh);

	AmmoCasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	AmmoCasingMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	AmmoCasingMesh->SetNotifyRigidBodyCollision(true);
	AmmoCasingMesh->SetSimulatePhysics(true);
	AmmoCasingMesh->SetEnableGravity(true);
	AmmoCasingMesh->SetUseCCD(true);

	SetCanBeDamaged(false);

	AActor::SetLifeSpan(LifeSpan);
}

void AAmmoCasing::BeginPlay()
{
	Super::BeginPlay();

	AmmoCasingMesh->OnComponentHit.AddDynamic(this, &AAmmoCasing::OnHit);

	AddRandomImpulse();
}

void AAmmoCasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	AmmoCasingMesh->SetNotifyRigidBodyCollision(false);
}

void AAmmoCasing::AddRandomImpulse()
{
	FVector ImpulseVector = GetActorForwardVector();
	ImpulseVector += GetActorUpVector() * FMath::RandRange(-EjectionRotationSpread, EjectionRotationSpread);
	ImpulseVector *= FMath::RandRange(EjectionImpulseMin, EjectionImpulseMax);
	AmmoCasingMesh->AddImpulseAtLocation(ImpulseVector, GetActorLocation() + FMath::VRand());
}
