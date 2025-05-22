// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportZone.generated.h"

class UBoxComponent;
class ATargetPoint;

UCLASS()
class BLASTER_API ATeleportZone : public AActor
{
	GENERATED_BODY()

public:
	ATeleportZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                               const FHitResult& SweepResult);

private:
	UPROPERTY(EditAnywhere, Category = "Teleport Zone Properties")
	TObjectPtr<ATargetPoint> Target;

	UPROPERTY(EditAnywhere, Category = "Teleport Zone Properties")
	TObjectPtr<UBoxComponent> OverlapBox;
};
