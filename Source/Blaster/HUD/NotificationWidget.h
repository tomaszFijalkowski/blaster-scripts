// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Blueprint/UserWidget.h"
#include "NotificationWidget.generated.h"

class UImage;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class BLASTER_API UNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEliminationText(const APlayerState* EliminatingPlayer, const APlayerState* AssistingPlayer,
	                        const APlayerState* Victim, const bool bIsImportant,
	                        const bool bSelfElimination);
	void SetFlagCaptureText(const APlayerState* ScoringPlayer);
	void PlayNotificationAnimation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NotificationBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> NotificationBackground;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NotificationText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> NotificationAnimation;
};
