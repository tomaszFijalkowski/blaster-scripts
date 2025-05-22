// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnnouncementWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class BLASTER_API UAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> AnnouncementBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnnouncementText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AnnouncementValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> InfoBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> TopPlayersBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> FirstPlaceBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FirstPlaceScore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FirstPlaceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> SecondPlaceBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SecondPlaceScore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SecondPlaceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> ThirdPlaceBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ThirdPlaceScore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ThirdPlaceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> WinningTeamBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> BlueTeamWinsBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RedTeamWinsBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DrawBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> BlueTeamScoreBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BlueTeamScoreLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BlueTeamScoreRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RedTeamScoreBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RedTeamScoreLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RedTeamScoreRight;
};
