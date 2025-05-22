// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "BlasterGameUserSettings.generated.h"

UCLASS()
class BLASTER_API UBlasterGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UBlasterGameUserSettings* GetGameUserSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMouseSensitivity(float Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetPostProcessingEnabled(bool bEnabled);

protected:
	UPROPERTY(Config)
	float MouseSensitivity = 1.f;

	UPROPERTY(Config)
	float MasterVolume = 0.5f;

	UPROPERTY(Config)
	bool EnablePostProcessing = true;

public:
	FORCEINLINE float GetMouseSensitivity() const { return MouseSensitivity; }
	FORCEINLINE float GetMasterVolume() const { return MasterVolume; }
	FORCEINLINE bool IsPostProcessingEnabled() const { return EnablePostProcessing; }
};
