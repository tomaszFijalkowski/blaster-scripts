// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

class UTextBlock;

UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDisplayText(const FString& TextToDisplay);
	void ToggleWithFade(bool bIsVisible);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(const APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerName(const APawn* InPawn);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DisplayText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeInAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeOutAnimation;

private:
	void PlayFadeInAnimation();
	void PlayFadeOutAnimation();

	bool bIsCurrentlyVisible = false;
};
