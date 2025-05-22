// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FlagResetWidget.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardRowWidget.generated.h"

class UTextBlock;

UCLASS()
class BLASTER_API UScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowCapturesText(bool bShow, float PlayerNameTextMinWidth, float CapturesTextMinWidth);
	void SetPlayerInfo(const FString& PlayerName, int32 Captures, int32 Score, int32 Kills, int32 Deaths, int32 Ping,
	                   ETeam Team, bool bIsLocalPlayer, int32 TopPlayerRank);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BadgeImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CapturesText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AssistsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefeatsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PingText;
};
