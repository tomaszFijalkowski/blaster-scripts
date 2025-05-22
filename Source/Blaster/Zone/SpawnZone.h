// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnZone.generated.h"

enum class ETeam : uint8;
class UBoxComponent;

UCLASS()
class BLASTER_API ASpawnZone : public AActor
{
	GENERATED_BODY()

public:
	ASpawnZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                               const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UPROPERTY(EditAnywhere, Category = "Spawn Zone Properties")
	ETeam Team;

	UPROPERTY(EditAnywhere, Category = "Spawn Zone Properties")
	TObjectPtr<UBoxComponent> OverlapBox;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE TObjectPtr<UBoxComponent> GetOverlapBox() const { return OverlapBox; }
};
