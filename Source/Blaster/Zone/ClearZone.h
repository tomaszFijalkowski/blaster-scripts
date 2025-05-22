// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClearZone.generated.h"

class UBoxComponent;

UCLASS()
class BLASTER_API AClearZone : public AActor
{
	GENERATED_BODY()

public:
	AClearZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                               const FHitResult& SweepResult);

private:
	UPROPERTY(EditAnywhere, Category = "Clear Zone Properties")
	TObjectPtr<UBoxComponent> OverlapBox;
};
