// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Components/TimelineComponent.h"
#include "Flag.generated.h"

#define MAX_DROP_DISTANCE 4000.f
#define MIN_CLEARANCE_RADIUS 50.f

class UFlagResetWidget;

UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	AFlag();
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool CanBeDropped(bool bPerformDropLineTrace) override;
	virtual void Dropped() override;
	virtual void ShowPickupWidget(bool bShowWidget) override;

	virtual FVector GetPickupSocketLocation() const override;

	void ResetFlag();

protected:
	virtual void BeginPlay() override;

	virtual void HandleWeaponInitialState() override;
	virtual void HandleWeaponEquippedState() override;
	virtual void HandleWeaponDroppedState() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleWeaponDroppedState(FVector AuthInitialDropLocation, FRotator AuthInitialDropRotation,
	                                       FVector AuthTargetDropLocation, FRotator AuthTargetDropRotation);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> FlagMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DropTimeline;

	UPROPERTY(EditAnywhere, Category = "Drop")
	TObjectPtr<UCurveFloat> DropCurve;

	UPROPERTY(EditAnywhere, Category = "Drop")
	float DropPlayRate = 2.f;

	UPROPERTY(EditAnywhere, Category = "Drop")
	int32 DropOffset = 30.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetFlag();

	UFUNCTION()
	void HandleDropProgress(float Value);

	UFUNCTION()
	void OnDropFinished();

	FVector GetDropLineTrace();

	FVector InitialDropLocation;
	FRotator InitialDropRotation;

	FVector TargetDropLocation;
	FRotator TargetDropRotation;

	FTransform InitialTransform;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	TObjectPtr<UCurveFloat> WidgetScaleCurve;

	UPROPERTY(VisibleAnywhere, Category = "Flag Properties", meta = (AllowPrivateAccess = true))
	TObjectPtr<UWidgetComponent> FlagResetWidgetComponent;

	UPROPERTY()
	TObjectPtr<UFlagResetWidget> FlagResetWidget;

	UPROPERTY(EditAnywhere, Category = "Flag Properties")
	TObjectPtr<USoundBase> FlagResetSound;

	bool bIsWithinFlagResetWidgetVisibilityDistance = false;

	UPROPERTY(VisibleAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetZOffset = 0.f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetZOffsetClose = 6.f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetZOffsetFar = 200.f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetScaleClose = 1.f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetScaleFar = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetTranslationYFar = -100.f;

	UPROPERTY(EditAnywhere, Category = "Flag Reset Widget")
	float FlagResetWidgetVisibilityDistance = 16000.f;

	UPROPERTY(EditAnywhere, Category = "Flag Properties")
	float FlagResetTime = 30.f;

	FTimerHandle FlagResetTimer;

	UPROPERTY(Replicated)
	float FlagResetServerStartTime = 0.f;

	FTimerHandle UpdateFlagResetWidgetTimer;

	void SetFlagResetWidget();
	void SetFlagResetWidgetComponentPosition();
	void ShowFlagResetWidget(const bool bShowWidget);

	void UpdateFlagResetWidgetDistance();
	void UpdateFlagResetRemainingTime();

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> LocallyControlledCharacter;

	ABlasterCharacter* GetLocallyControlledCharacter() const;

public:
	FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }
	FORCEINLINE TObjectPtr<UStaticMeshComponent> GetFlagMesh() const { return FlagMesh; }
};
