// Fill out your copyright notice in the Description page of Project Settings.

#include "NotificationWidget.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UNotificationWidget::SetEliminationText(const APlayerState* EliminatingPlayer, const APlayerState* AssistingPlayer,
                                             const APlayerState* Victim, const bool bIsImportant,
                                             const bool bSelfElimination)
{
	if (NotificationBackground && NotificationText && Victim)
	{
		const FLinearColor BackgroundColor = bIsImportant
			                                     ? FLinearColor::FromSRGBColor(FColor(229, 85, 85, 255))
			                                     : FLinearColor::FromSRGBColor(FColor(236, 119, 119, 255));
		NotificationBackground->SetBrushTintColor(FSlateColor(BackgroundColor));

		FString Text;
		if (EliminatingPlayer)
		{
			Text = EliminatingPlayer->GetPlayerName();

			if (AssistingPlayer)
			{
				Text += TEXT("  +  ") + AssistingPlayer->GetPlayerName();
			}

			Text += bSelfElimination ? TEXT("  finished off  ") : TEXT("  killed  ");
			Text += Victim->GetPlayerName();
		}
		else
		{
			Text = FString::Printf(TEXT("%s  bid farewell, cruel world!"), *Victim->GetPlayerName());
		}

		NotificationText->SetText(FText::FromString(Text));

		const FLinearColor TextColor = bIsImportant
			                               ? FLinearColor::White
			                               : FLinearColor::FromSRGBColor(FColor(50, 50, 50, 255));
		NotificationText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

void UNotificationWidget::SetFlagCaptureText(const APlayerState* ScoringPlayer)
{
	if (NotificationBackground && NotificationText && ScoringPlayer)
	{
		const ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(ScoringPlayer);
		const ETeam ScoringTeam = BlasterPlayerState ? BlasterPlayerState->GetTeam() : ETeam::ET_NoTeam;
		const FLinearColor BackgroundColor = ScoringTeam == ETeam::ET_BlueTeam
			                                     ? FLinearColor::FromSRGBColor(FColor(70, 124, 229, 255))
			                                     : FLinearColor::FromSRGBColor(FColor(229, 53, 61, 255));
		NotificationBackground->SetBrushTintColor(FSlateColor(BackgroundColor));

		const FString Text = FString::Printf(TEXT("%s  captured the flag!"), *ScoringPlayer->GetPlayerName());
		NotificationText->SetText(FText::FromString(Text));

		const FLinearColor TextColor = FLinearColor::White;
		NotificationText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

void UNotificationWidget::PlayNotificationAnimation()
{
	if (NotificationAnimation && !IsAnimationPlaying(NotificationAnimation))
	{
		PlayAnimation(NotificationAnimation, 0.f);
	}
}
