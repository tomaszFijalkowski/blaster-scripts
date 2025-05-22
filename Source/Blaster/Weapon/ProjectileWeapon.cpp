// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Projectile.h"
#include "Blaster/Constants/Constants.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	UWorld* World = GetWorld();
	const TObjectPtr<USkeletalMeshComponent> SkeletalMesh = GetWeaponMesh();
	const USkeletalMeshSocket* MuzzleFlashSocket = SkeletalMesh->GetSocketByName(Socket::MuzzleFlash);
	if (World && MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(SkeletalMesh);

		// From muzzle flash socket to target hit location
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = ToTarget.Rotation();

		APawn* InstigatorPawn = Cast<APawn>(GetOwner());

		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = InstigatorPawn;
		SpawnParams.Owner = GetOwner();

		AProjectile* SpawnedProjectile;

		if (bUseServerSideRewind)
		{
			// server, weapon using SSR
			if (InstigatorPawn->HasAuthority())
			{
				// server, host - spawn replicated projectile
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(
						ProjectileClass,
						SocketTransform.GetLocation(),
						TargetRotation,
						SpawnParams
					);
					SpawnedProjectile->SetUseServerSideRewind(false);
					SpawnedProjectile->SetDamageProperties(
						Damage,
						HeadshotDamage,
						MinDamage,
						DamageInnerRadius,
						DamageOuterRadius
					);
				}
				// server, not locally controlled - spawn non-replicated projectile, use SSR
				else
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(
						LocalProjectileClass,
						SocketTransform.GetLocation(),
						TargetRotation,
						SpawnParams
					);
					SpawnedProjectile->SetUseServerSideRewind(true);
				}
			}
			// client, weapon using SSR
			else
			{
				// client, locally controlled - spawn non-replicated projectile, use SSR
				if (InstigatorPawn->IsLocallyControlled())
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(
						LocalProjectileClass,
						SocketTransform.GetLocation(),
						TargetRotation,
						SpawnParams
					);
					SpawnedProjectile->SetUseServerSideRewind(true);
					SpawnedProjectile->SetTraceStart(SocketTransform.GetLocation());
					const FVector InitialVelocity =
						SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed();
					SpawnedProjectile->SetInitialVelocity(InitialVelocity);
					SpawnedProjectile->SetDamageProperties(
						Damage,
						HeadshotDamage,
						MinDamage,
						DamageInnerRadius,
						DamageOuterRadius
					);
				}
				// client, not locally controlled - spawn non-replicated projectile, no SSR
				else
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(
						LocalProjectileClass,
						SocketTransform.GetLocation(),
						TargetRotation,
						SpawnParams
					);
					SpawnedProjectile->SetUseServerSideRewind(false);
				}
			}
		}
		else
		{
			// server, weapon not using SSR
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
				SpawnedProjectile->SetUseServerSideRewind(false);
				SpawnedProjectile->SetDamageProperties(
					Damage,
					HeadshotDamage,
					MinDamage,
					DamageInnerRadius,
					DamageOuterRadius
				);
			}
		}
	}
}
