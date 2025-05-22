// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponSpawnPoint.generated.h"

class AWeapon;

UCLASS()
class BLASTER_API AWeaponSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AWeaponSpawnPoint();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void StartSpawnWeaponTimer();

	void SpawnWeaponTimerFinished();
	void SpawnWeapon();

	UPROPERTY(EditAnywhere, Category = "Weapon Spawn Point Properties")
	TSubclassOf<AWeapon> WeaponClass;

	UPROPERTY()
	TObjectPtr<AWeapon> SpawnedWeapon;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Spawn Point Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditAnywhere, Category = "Weapon Spawn Point Properties")
	float SpawnWeaponTime = 30.f;

	FTimerHandle SpawnWeaponTimer;
};
