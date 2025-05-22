// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/Actor.h"
#include "FlagZone.generated.h"

class UNiagaraSystem;
class USphereComponent;

UCLASS()
class BLASTER_API AFlagZone : public AActor
{
	GENERATED_BODY()

public:
	AFlagZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayFlagCaptureEffects(const FTransform& FlagTransform);

	UPROPERTY(EditAnywhere, Category = "Flag Zone  Properties")
	TObjectPtr<UNiagaraSystem> FlagCaptureEffect;

	UPROPERTY(EditAnywhere, Category = "Flag Zone Properties")
	TObjectPtr<USoundBase> FlagCaptureSound;

	UPROPERTY(EditAnywhere, Category = "Flag Zone Properties")
	ETeam Team;

	UPROPERTY(EditAnywhere, Category = "Flag Zone Properties")
	TObjectPtr<USphereComponent> OverlapSphere;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
