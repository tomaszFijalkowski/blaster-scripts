// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EliminationZone.generated.h"

class UBoxComponent;

UCLASS()
class BLASTER_API AEliminationZone : public AActor
{
	GENERATED_BODY()

public:
	AEliminationZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                               const FHitResult& SweepResult);

private:
	UPROPERTY(EditAnywhere, Category = "Elimination Zone Properties")
	TObjectPtr<UBoxComponent> OverlapBox;
};
