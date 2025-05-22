// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UHorizontalBox;
class UTextBlock;
class UProgressBar;
class UImage;

UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	void PlayMatchCountdownAnimation();
	void PlayPingAmountFadeInAnimation();
	void PlayHighPingAnimation();
	void StopHighPingAnimation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RespawnBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RespawnTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchCountdownText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> MatchCountdownAnimation;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> MatchCountdownAnimationSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GrenadesText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PingAmount;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> PingAmountFadeInAnimation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HighPingImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> HighPingImageAnimation;

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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ReloadProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BuffDurationBar1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BuffDurationBar2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BuffDurationBar3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BuffDurationBar4;
};
