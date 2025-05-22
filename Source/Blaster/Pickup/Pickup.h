// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class ABlasterCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()

public:
	APickup();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult);

	void DeferredDestroy();

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	float Amplitude = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Hover Parameters")
	float TimeConstant = 5.f;

	bool bIsBeingDestroyed = false;

private:
	void CheckForOverlappingActors();

	void HandleHovering(float DeltaTime);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleDestroy(USceneComponent* ComponentToAttach);

	void HidePickup() const;
	void SpawnPickupEffect() const;
	void PlayPickupSound(USceneComponent* ComponentToAttach) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float RunningTime;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	float CheckForOverlappingActorsInterval = 0.1f;

	FTimerHandle CheckForOverlappingActorsTimer;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	TObjectPtr<USoundAttenuation> PickupSoundAttenuation;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	float PickupEffectOffsetZ = 35.f;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	TObjectPtr<UNiagaraSystem> PickupEffect;

	UPROPERTY(EditAnywhere, Category = "Pickup Properties")
	float DestroyTime = 5.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> PickupEffectComponent;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OverlappedCharacter;

	FTimerHandle DestroyTimer;
};
