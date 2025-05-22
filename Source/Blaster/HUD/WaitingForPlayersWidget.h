// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaitingForPlayersWidget.generated.h"

class UTextBlock;
class UHorizontalBox;

UCLASS()
class BLASTER_API UWaitingForPlayersWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void PlayMatchStartingAnimation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> WaitingForPlayersBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WaitingForPlayersNumber;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> MatchStartingBox;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> MatchStartingAnimation;
};
