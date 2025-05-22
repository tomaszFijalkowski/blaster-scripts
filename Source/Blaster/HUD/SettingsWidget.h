// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class USlider;
class UTextBlock;
class UMultiplayerSessionsSubsystem;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBackButtonClickedDelegate);

UCLASS()
class BLASTER_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnBackButtonClickedDelegate OnBackButtonClicked;

private:
	UFUNCTION()
	void OnMouseSensitivityChanged(float Value);

	UFUNCTION()
	void OnMouseSensitivityCaptureEnd();

	UFUNCTION()
	void OnMasterVolumeChanged(float Value);

	UFUNCTION()
	void OnMasterVolumeCaptureEnd();

	UFUNCTION()
	void OnPostProcessingChanged(float Value);

	UFUNCTION()
	void OnPostProcessingCaptureEnd();

	UFUNCTION()
	void OnBackButtonPressed();

	void UpdateMouseSensitivityText(float Value);
	void UpdateMasterVolumeText(float Value);
	void UpdatePostProcessingText(float Value);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MouseSensitivityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PostProcessingText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> PostProcessingSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;
};
