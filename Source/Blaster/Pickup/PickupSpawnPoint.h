// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class UNiagaraComponent;
class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

	void SpawnPickupTimerFinished();
	void SpawnPickup();

	UPROPERTY(EditAnywhere, Category = "Pickup Spawn Point Properties")
	TObjectPtr<UStaticMeshComponent> PickupPreviewMesh;

	UPROPERTY(EditAnywhere, Category = "Pickup Spawn Point Properties")
	TSubclassOf<APickup> PickupClass;

	UPROPERTY()
	TObjectPtr<APickup> SpawnedPickup;

private:
	UPROPERTY(EditAnywhere, Category = "Pickup Spawn Point Properties")
	float SpawnPickupTime = 60.f;

	FTimerHandle SpawnPickupTimer;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> PickupPreviewEffectComponent;
};
