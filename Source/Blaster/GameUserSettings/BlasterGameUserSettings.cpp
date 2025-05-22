// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameUserSettings.h"

UBlasterGameUserSettings* UBlasterGameUserSettings::GetGameUserSettings()
{
	if (GEngine)
	{
		return Cast<UBlasterGameUserSettings>(GEngine->GetGameUserSettings());
	}

	return nullptr;
}

void UBlasterGameUserSettings::SetMouseSensitivity(const float Sensitivity)
{
	MouseSensitivity = FMath::Clamp(Sensitivity, 0.05f, 5.f);
}

void UBlasterGameUserSettings::SetMasterVolume(const float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);
}

void UBlasterGameUserSettings::SetPostProcessingEnabled(const bool bEnabled)
{
	EnablePostProcessing = bEnabled;
}
