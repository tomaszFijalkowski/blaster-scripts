// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "AoeDamageType.h"
#include "HitscanWeapon.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystemInstanceController.h"
#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);

	CollisionBox->bReturnMaterialOnMove = true;

	SetCanBeDamaged(false);
}

void AProjectile::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::SetDamageProperties(const float InDamage, const float InHeadshotDamage, const float InMinDamage,
                                      const float InDamageInnerRadius, const float InDamageOuterRadius)
{
	Damage = InDamage;
	HeadshotDamage = InHeadshotDamage;
	MinDamage = InMinDamage;
	DamageInnerRadius = InDamageInnerRadius;
	DamageOuterRadius = InDamageOuterRadius;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TracerParticles)
	{
		const FVector InitialLocation = GetActorLocation();
		const FRotator InitialRotation = GetActorRotation();

		GetWorldTimerManager().SetTimerForNextTick([this, InitialLocation, InitialRotation]
		{
			TracerParticlesComponent = UGameplayStatics::SpawnEmitterAttached(
				TracerParticles,
				CollisionBox,
				FName(),
				InitialLocation,
				InitialRotation,
				EAttachLocation::KeepWorldPosition
			);

			if (bShouldHideTracerOnInit)
			{
				GetWorldTimerManager().SetTimerForNextTick([this]
				{
					if (TracerParticlesComponent)
					{
						TracerParticlesComponent->DeactivateSystem();
					}
				});
			}
		});
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

	if (AActor* InstigatorPawn = GetInstigator())
	{
		CollisionBox->IgnoreActorWhenMoving(InstigatorPawn, true);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	const bool bHitHead = Hit.BoneName.ToString() == Bone::HeadBone;

	if (const UPhysicalMaterial* PhysMaterial = Hit.PhysMaterial.Get())
	{
		MulticastOnHit(PhysMaterial->SurfaceType, bHitHead);
	}
	else
	{
		MulticastOnHit(SurfaceType_Default, bHitHead);
	}
}

void AProjectile::HideProjectile()
{
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}

	if (TracerParticlesComponent)
	{
		TracerParticlesComponent->DeactivateSystem();
	}
	else if (TracerParticles)
	{
		bShouldHideTracerOnInit = true;
	}
}

void AProjectile::MulticastOnHit_Implementation(const EPhysicalSurface PhysicalSurface, const bool bHitHead)
{
	SetImpactEffects(PhysicalSurface, bHitHead);
	PlayImpactEffects();
	StartDestroyTimer();
	HideProjectile();
}

bool AProjectile::ExplodeDamage()
{
	bool bAppliedDamage = false;

	const APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		if (AController* FiringController = FiringPawn->GetController())
		{
			TArray<AActor*> CharacterActors;
			TArray<AActor*> ActorsToIgnore;

			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlasterCharacter::StaticClass(), CharacterActors);

			for (AActor* CharacterActor : CharacterActors)
			{
				ABlasterCharacter* Character = Cast<ABlasterCharacter>(CharacterActor);
				if (Character && Character->IsEliminated())
				{
					ActorsToIgnore.Add(Character);
				}
			}

			bAppliedDamage = UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),
				DamageInnerRadius,
				DamageOuterRadius,
				1.f,
				UAoeDamageType::StaticClass(),
				ActorsToIgnore,
				this,
				FiringController
			);
		}
	}

	return bAppliedDamage;
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		const FVector OffsetX = GetActorForwardVector() * TrailSystemOffsetX;
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			NAME_None,
			GetActorLocation() + OffsetX,
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::SetImpactEffects(const EPhysicalSurface PhysicalSurface, const bool bHitHead)
{
	switch (PhysicalSurface)
	{
	case EPS_Player:
		ImpactParticlesToSpawn = PlayerImpactParticles;
		ImpactSoundToPlay = bHitHead ? HeadshotImpactSound : BodyshotImpactSound;
		break;
	case EPS_Metal:
		ImpactParticlesToSpawn = MetalImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	case EPS_Stone:
		ImpactParticlesToSpawn = StoneImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	case EPS_Wood:
		ImpactParticlesToSpawn = WoodImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
		break;
	default:
		ImpactParticlesToSpawn = StoneImpactParticles;
		ImpactSoundToPlay = DefaultImpactSound;
	}
}

void AProjectile::PlayImpactEffects() const
{
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}

	if (ImpactSoundToPlay)
	{
		PlayImpactSound();
	}

	if (ImpactParticlesToSpawn)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticlesToSpawn,
			GetActorLocation(),
			GetActorRotation(),
			FVector(ImpactParticlesScale)
		);
	}
}

void AProjectile::PlayImpactSound() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const bool bIsLocallyControlled = OwnerPawn && OwnerPawn->IsLocallyControlled();
	const bool bHitPlayer = ImpactSoundToPlay == HeadshotImpactSound || ImpactSoundToPlay == BodyshotImpactSound;

	if (bIsLocallyControlled && bHitPlayer)
	{
		UGameplayStatics::PlaySound2D(this, ImpactSoundToPlay, SOUND_2D_VOLUME_MULTIPLIER);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSoundToPlay, GetActorLocation());
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime,
		false
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}
