// Fill out your copyright notice in the Description page of Project Settings.

#include "DamageWidget.h"
#include "Components/TextBlock.h"

void UDamageWidget::SetDamageNumber(const float Number, const bool bIsHeadshot)
{
	if (DamageNumberText)
	{
		const FString DamageNumberString = FString::Printf(TEXT("%d"), FMath::CeilToInt(Number));
		DamageNumberText->SetText(FText::FromString(DamageNumberString));

		PlayDamageNumberAnimation();

		if (bIsHeadshot)
		{
			PlayHeadshotAnimation();
		}
		else
		{
			PlayBodyshotAnimation();
		}
	}
}

void UDamageWidget::PlayDamageNumberAnimation()
{
	if (DamageNumberAnimation)
	{
		if (IsAnimationPlaying(DamageNumberAnimation))
		{
			StopAnimation(DamageNumberAnimation);
		}

		PlayAnimation(DamageNumberAnimation);
	}
}

void UDamageWidget::PlayBodyshotAnimation()
{
	if (BodyshotScaleAnimation)
	{
		if (HeadshotScaleAnimation && IsAnimationPlaying(HeadshotScaleAnimation))
		{
			StopAnimation(HeadshotScaleAnimation);
		}

		if (IsAnimationPlaying(BodyshotScaleAnimation))
		{
			StopAnimation(BodyshotScaleAnimation);
		}

		PlayAnimation(BodyshotScaleAnimation);
	}
}

void UDamageWidget::PlayHeadshotAnimation()
{
	if (HeadshotScaleAnimation)
	{
		if (BodyshotScaleAnimation && IsAnimationPlaying(BodyshotScaleAnimation))
		{
			StopAnimation(BodyshotScaleAnimation);
		}

		if (IsAnimationPlaying(HeadshotScaleAnimation))
		{
			StopAnimation(HeadshotScaleAnimation);
		}

		PlayAnimation(HeadshotScaleAnimation);
	}

	if (HeadshotColorAnimation)
	{
		if (IsAnimationPlaying(HeadshotColorAnimation))
		{
			StopAnimation(HeadshotColorAnimation);
		}

		PlayAnimation(HeadshotColorAnimation);
	}
}
