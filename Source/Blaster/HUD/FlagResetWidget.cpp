// Fill out your copyright notice in the Description page of Project Settings.

#include "FlagResetWidget.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UFlagResetWidget::SetFlagResetImage(const ETeam Team)
{
	if (FlagResetImage)
	{
		const FLinearColor ImageColor = Team == ETeam::ET_BlueTeam
			                                ? FLinearColor::FromSRGBColor(FColor(70, 124, 229, 255))
			                                : FLinearColor::FromSRGBColor(FColor(229, 53, 61, 255));
		FlagResetImage->SetColorAndOpacity(ImageColor);
	}
}

void UFlagResetWidget::SetFlagResetRemainingTime(const float RemainingTime)
{
	if (FlagResetRemainingTime)
	{
		const FString RemainingTimeText = FString::Printf(TEXT("%.1fs"), RemainingTime);
		FlagResetRemainingTime->SetText(FText::FromString(RemainingTimeText));
	}
}
