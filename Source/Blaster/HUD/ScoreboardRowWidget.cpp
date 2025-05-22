// Fill out your copyright notice in the Description page of Project Settings.

#include "ScoreboardRowWidget.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UScoreboardRowWidget::ShowCapturesText(const bool bShow, const float PlayerNameTextMinWidth,
                                            const float CapturesTextMinWidth)
{
	if (PlayerNameText && CapturesText)
	{
		if (bShow)
		{
			PlayerNameText->SetMinDesiredWidth(PlayerNameTextMinWidth - CapturesTextMinWidth);
			CapturesText->SetMinDesiredWidth(CapturesTextMinWidth);
			CapturesText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			PlayerNameText->SetMinDesiredWidth(PlayerNameTextMinWidth);
			CapturesText->SetMinDesiredWidth(0.f);
			CapturesText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UScoreboardRowWidget::SetPlayerInfo(const FString& PlayerName, const int32 Captures, const int32 Score,
                                         const int32 Kills, const int32 Deaths, const int32 Ping, const ETeam Team,
                                         const bool bIsLocalPlayer, const int32 TopPlayerRank = -1)
{
	if (BadgeImage)
	{
		switch (Team)
		{
		case ETeam::ET_BlueTeam:
			BadgeImage->SetVisibility(ESlateVisibility::Visible);
			BadgeImage->SetColorAndOpacity(FColor(70, 124, 229, 255));
			break;
		case ETeam::ET_RedTeam:
			BadgeImage->SetVisibility(ESlateVisibility::Visible);
			BadgeImage->SetColorAndOpacity(FColor(229, 53, 61, 255));
			break;
		default:
			if (TopPlayerRank == 0)
			{
				BadgeImage->SetVisibility(ESlateVisibility::Visible);
				BadgeImage->SetColorAndOpacity(FColor(247, 245, 86, 255));
			}
			else if (TopPlayerRank == 1)
			{
				BadgeImage->SetVisibility(ESlateVisibility::Visible);
				BadgeImage->SetColorAndOpacity(FColor(203, 203, 203, 255));
			}
			else if (TopPlayerRank == 2)
			{
				BadgeImage->SetVisibility(ESlateVisibility::Visible);
				BadgeImage->SetColorAndOpacity(FColor(166, 131, 102, 255));
			}
			else
			{
				BadgeImage->SetVisibility(ESlateVisibility::Collapsed);
			}
			break;
		}
	}

	const FLinearColor TextColor = bIsLocalPlayer
		                               ? FLinearColor::White
		                               : FLinearColor::FromSRGBColor(FColor(192, 192, 192, 255));
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerName));
		PlayerNameText->SetColorAndOpacity(TextColor);
	}

	if (CapturesText)
	{
		CapturesText->SetText(FText::AsNumber(Captures));
		CapturesText->SetColorAndOpacity(TextColor);
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::AsNumber(Score));
		ScoreText->SetColorAndOpacity(TextColor);
	}

	if (AssistsText)
	{
		AssistsText->SetText(FText::AsNumber(Kills));
		AssistsText->SetColorAndOpacity(TextColor);
	}

	if (DefeatsText)
	{
		DefeatsText->SetText(FText::AsNumber(Deaths));
		DefeatsText->SetColorAndOpacity(TextColor);
	}

	if (PingText)
	{
		PingText->SetText(FText::AsNumber(Ping));
		PingText->SetColorAndOpacity(TextColor);
	}
}
