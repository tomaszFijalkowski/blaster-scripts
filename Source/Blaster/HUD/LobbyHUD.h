// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

class UWaitingForPlayersWidget;

UCLASS()
class BLASTER_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()

public:
	void AddWaitingForPlayersWidget();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(EditAnywhere, Category = "Waiting for players Widget")
	TSubclassOf<UUserWidget> WaitingForPlayersWidgetClass;

	UPROPERTY()
	TObjectPtr<UWaitingForPlayersWidget> WaitingForPlayersWidget;

public:
	FORCEINLINE TObjectPtr<UWaitingForPlayersWidget> GetWaitingForPlayersWidget() const
	{
		return WaitingForPlayersWidget;
	}
};
