// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsWidget.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/GameUserSettings/BlasterGameUserSettings.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerController/LobbyPlayerController.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings();
	if (Settings == nullptr)
	{
		return;
	}

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMouseSensitivityChanged);
		MouseSensitivitySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsWidget::OnMouseSensitivityCaptureEnd);

		MouseSensitivitySlider->SetValue(Settings->GetMouseSensitivity());
	}

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMasterVolumeChanged);
		MasterVolumeSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsWidget::OnMasterVolumeCaptureEnd);

		MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	}

	if (PostProcessingSlider)
	{
		PostProcessingSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnPostProcessingChanged);
		PostProcessingSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsWidget::OnPostProcessingCaptureEnd);

		PostProcessingSlider->SetValue(Settings->IsPostProcessingEnabled() ? 1.f : 0.f);
	}

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &USettingsWidget::OnBackButtonPressed);
	}
}

void USettingsWidget::NativeDestruct()
{
	if (MouseSensitivitySlider && MouseSensitivitySlider->OnValueChanged.IsBound())
	{
		MouseSensitivitySlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::OnMouseSensitivityChanged);
		MouseSensitivitySlider->OnMouseCaptureEnd.RemoveDynamic(this, &USettingsWidget::OnMouseSensitivityCaptureEnd);
	}

	if (MasterVolumeSlider && MasterVolumeSlider->OnValueChanged.IsBound())
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::OnMasterVolumeChanged);
		MasterVolumeSlider->OnMouseCaptureEnd.RemoveDynamic(this, &USettingsWidget::OnMasterVolumeCaptureEnd);
	}

	if (PostProcessingSlider && PostProcessingSlider->OnValueChanged.IsBound())
	{
		PostProcessingSlider->OnValueChanged.RemoveDynamic(this, &USettingsWidget::OnPostProcessingChanged);
		PostProcessingSlider->OnMouseCaptureEnd.RemoveDynamic(this, &USettingsWidget::OnPostProcessingCaptureEnd);
	}

	if (BackButton && BackButton->OnClicked.IsBound())
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsWidget::OnBackButtonPressed);
	}

	if (UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		Settings->SaveSettings();
	}

	Super::NativeDestruct();
}

void USettingsWidget::OnMouseSensitivityChanged(const float Value)
{
	UpdateMouseSensitivityText(Value);
}

void USettingsWidget::OnMouseSensitivityCaptureEnd()
{
	if (UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		Settings->SetMouseSensitivity(MouseSensitivitySlider->GetValue());
		Settings->ApplySettings(false);
	}
}

void USettingsWidget::OnMasterVolumeChanged(const float Value)
{
	UpdateMasterVolumeText(Value);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBlasterGameInstance* BlasterGameInstance = Cast<UBlasterGameInstance>(GameInstance))
		{
			BlasterGameInstance->SetMasterVolume(Value);
		}
	}
}

void USettingsWidget::OnMasterVolumeCaptureEnd()
{
	if (UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		Settings->SetMasterVolume(MasterVolumeSlider->GetValue());
		Settings->ApplySettings(false);
	}
}

void USettingsWidget::OnPostProcessingChanged(const float Value)
{
	UpdatePostProcessingText(Value);

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (ABlasterPlayerController* BlasterPC = Cast<ABlasterPlayerController>(PlayerController))
		{
			BlasterPC->SetPostProcessingEnabled(Value > 0.5f);
		}

		if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PlayerController))
		{
			LobbyPC->SetPostProcessingEnabled(Value > 0.5f);
		}
	}
}

void USettingsWidget::OnPostProcessingCaptureEnd()
{
	if (UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		Settings->SetPostProcessingEnabled(PostProcessingSlider->GetValue() > 0.5f);
		Settings->ApplySettings(false);
	}
}

void USettingsWidget::OnBackButtonPressed()
{
	OnBackButtonClicked.Broadcast();
}

void USettingsWidget::UpdateMouseSensitivityText(const float Value)
{
	if (MouseSensitivityText)
	{
		const FString MouseSensitivityString = FString::Printf(TEXT("Mouse sensitivity: %.2f"), Value);
		MouseSensitivityText->SetText(FText::FromString(MouseSensitivityString));
	}
}

void USettingsWidget::UpdateMasterVolumeText(const float Value)
{
	if (MasterVolumeText)
	{
		const FString MasterVolumeString = FString::Printf(
			TEXT("Master volume: %d%%"), FMath::RoundToInt(Value * 100.f));
		MasterVolumeText->SetText(FText::FromString(MasterVolumeString));
	}
}

void USettingsWidget::UpdatePostProcessingText(const float Value)
{
	if (PostProcessingText)
	{
		const FString PostProcessingState = Value > 0.5f ? TEXT("ON") : TEXT("OFF");
		const FString PostProcessingString = FString::Printf(TEXT("Post processing: %s"), *PostProcessingState);
		PostProcessingText->SetText(FText::FromString(PostProcessingString));
	}
}
