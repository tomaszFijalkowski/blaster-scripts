// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoCasing.generated.h"

UCLASS()
class BLASTER_API AAmmoCasing : public AActor
{
	GENERATED_BODY()

public:
	AAmmoCasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

private:
	void AddRandomImpulse();

	UPROPERTY(VisibleAnywhere, Category = "Ammo Casing Properties")
	TObjectPtr<UStaticMeshComponent> AmmoCasingMesh;

	UPROPERTY(EditAnywhere, Category = "Ammo Casing Properties")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Ammo Casing Properties")
	float EjectionImpulseMin = 10.f;

	UPROPERTY(EditAnywhere, Category = "Ammo Casing Properties")
	float EjectionImpulseMax = 30.f;

	UPROPERTY(EditAnywhere, Category = "Ammo Casing Properties")
	float EjectionRotationSpread = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Ammo Casing Properties")
	float LifeSpan = 3.f;
};
