// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameInstance.h"
#include "AudioDevice.h"
#include "Blaster/GameUserSettings/BlasterGameUserSettings.h"

void UBlasterGameInstance::Init()
{
	Super::Init();

	if (const UBlasterGameUserSettings* Settings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		SetMasterVolume(Settings->GetMasterVolume());
	}
}

void UBlasterGameInstance::SetMasterVolume(const float Volume)
{
	if (GEngine && MasterSoundClass)
	{
		FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
		if (AudioDevice.IsValid())
		{
			MasterSoundClass->Properties.Volume = Volume;
		}
	}
}
