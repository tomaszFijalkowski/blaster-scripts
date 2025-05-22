// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileRocket.h"
#include "RocketMovementComponent.h"
#include "Blaster/Blaster.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->InitialSpeed = InitialSpeed;
	RocketMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	const FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialSpeed))
	{
		if (RocketMovementComponent)
		{
			RocketMovementComponent->InitialSpeed = InitialSpeed;
			RocketMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	if (APawn* InstigatingPawn = GetInstigator())
	{
		if (UPrimitiveComponent* ProjectileMeshComponent = Cast<UPrimitiveComponent>(ProjectileMesh))
		{
			ProjectileMeshComponent->IgnoreActorWhenMoving(InstigatingPawn, true);
		}
	}

	SpawnTrailSystem();

	if (ProjectileLoopSound && ProjectileLoopSoundAttenuation)
	{
		ProjectileLoopSoundComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoopSound,
			GetRootComponent(),
			NAME_None,
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			ProjectileLoopSoundAttenuation,
			nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	if (HasAuthority())
	{
		const bool bHitPlayer = ExplodeDamage();
		MulticastExplodeRocket(Hit.ImpactPoint, bHitPlayer);
	}
	else // Client prediction if the server hasn't exploded the rocket yet
	{
		ExplodeRocket();
	}
}

void AProjectileRocket::HideProjectile()
{
	Super::HideProjectile();

	if (ProjectileLoopSoundComponent && ProjectileLoopSoundComponent->IsPlaying())
	{
		ProjectileLoopSoundComponent->Stop();
	}
}

void AProjectileRocket::ExplodeRocket(const FVector& HitLocation, const bool bHitPlayer)
{
	if (bHitPlayer)
	{
		SetImpactEffects(EPS_Player, false);
	}
	else
	{
		SetImpactEffects(SurfaceType_Default);
	}

	if (bAlreadyExploded)
	{
		// Client can't determine if the explosion hit a player or not,
		// so we need to allow replaying the impact sound if it comes through the multicast
		if (bHitPlayer && !HasAuthority())
		{
			PlayImpactSound();
		}

		return;
	}

	bAlreadyExploded = true;

	// Correct client's rocket location to match server's location
	if (!HitLocation.IsNearlyZero() && !HasAuthority())
	{
		SetActorLocation(HitLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	PlayImpactEffects();
	StartDestroyTimer();
	HideProjectile();
}

void AProjectileRocket::MulticastExplodeRocket_Implementation(const FVector_NetQuantize HitLocation,
                                                              const bool bHitPlayer)
{
	ExplodeRocket(HitLocation, bHitPlayer);
}
