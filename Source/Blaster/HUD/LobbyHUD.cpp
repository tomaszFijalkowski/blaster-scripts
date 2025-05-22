// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyHUD.h"
#include "WaitingForPlayersWidget.h"

class UWaitingForPlayersWidget;

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ALobbyHUD::AddWaitingForPlayersWidget()
{
	PlayerController = PlayerController == nullptr ? GetOwningPlayerController() : PlayerController.Get();
	if (PlayerController && WaitingForPlayersWidgetClass && WaitingForPlayersWidget == nullptr)
	{
		WaitingForPlayersWidget =
			CreateWidget<UWaitingForPlayersWidget>(PlayerController, WaitingForPlayersWidgetClass);
		if (WaitingForPlayersWidget)
		{
			WaitingForPlayersWidget->AddToViewport();
		}
	}
}
