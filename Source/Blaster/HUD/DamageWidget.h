// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageWidget.generated.h"

class UTextBlock;

UCLASS()
class BLASTER_API UDamageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDamageNumber(float Number, bool bIsHeadshot);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageNumberText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> DamageNumberAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> BodyshotScaleAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> HeadshotScaleAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> HeadshotColorAnimation;

private:
	void PlayDamageNumberAnimation();
	void PlayBodyshotAnimation();
	void PlayHeadshotAnimation();
};
