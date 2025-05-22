// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileGrenade.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->bShouldBounce = true;
}

#if WITH_EDITOR
void AProjectileGrenade::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	const FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileGrenade, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay(); // Skip AProjectile::BeginPlay() to prevent the projectile from being destroyed immediately

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);

	if (AActor* InstigatorPawn = GetInstigator())
	{
		CollisionBox->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	SpawnTrailSystem();
	StartDetonateTimer();
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	const bool bBounceSoundIsPlaying = BounceSoundComponent && BounceSoundComponent->IsPlaying();
	if (BounceSound && BounceSoundAttenuation && !bBounceSoundIsPlaying)
	{
		BounceSoundComponent = UGameplayStatics::SpawnSoundAttached(
			BounceSound,
			GetRootComponent(),
			NAME_None,
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			BounceSoundAttenuation,
			nullptr,
			false
		);
	}

	if (ImpactVelocity.Size() < BounceVelocityThreshold)
	{
		StopGrenade();
	}

	if (bShouldDetonateOnImpact)
	{
		const AActor* HitActor = Cast<ABlasterCharacter>(ImpactResult.GetActor());
		if (HitActor && HitActor != GetOwner())
		{
			GetWorldTimerManager().ClearTimer(DetonateTimer);

			if (HasAuthority())
			{
				const bool bHitPlayer = ExplodeDamage();
				MulticastExplodeGrenade(ImpactResult.ImpactPoint, bHitPlayer);
			}
			else // Client prediction if the server hasn't exploded the grenade yet
			{
				ExplodeGrenade();
			}
		}
	}
}

void AProjectileGrenade::StartDetonateTimer()
{
	GetWorldTimerManager().SetTimer(
		DetonateTimer,
		[this]
		{
			DetonateTimerFinished();
		},
		DetonateTime,
		false
	);
}

void AProjectileGrenade::DetonateTimerFinished()
{
	if (HasAuthority())
	{
		const bool bHitPlayer = ExplodeDamage();
		MulticastExplodeGrenade(FVector::ZeroVector, bHitPlayer);
	}
	else // Client prediction if the server hasn't exploded the grenade yet
	{
		ExplodeGrenade();
	}
}

void AProjectileGrenade::ExplodeGrenade(const FVector& HitLocation, const bool bHitPlayer)
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

	// Correct client's grenade location to match server's location
	if (!HitLocation.IsNearlyZero() && !HasAuthority())
	{
		SetActorLocation(HitLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	PlayImpactEffects();
	StartDestroyTimer();
	HideProjectile();
}

void AProjectileGrenade::StopGrenade()
{
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->Velocity = FVector::ZeroVector;

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
}

void AProjectileGrenade::MulticastExplodeGrenade_Implementation(const FVector_NetQuantize HitLocation,
                                                                const bool bHitPlayer)
{
	ExplodeGrenade(HitLocation, bHitPlayer);
}
