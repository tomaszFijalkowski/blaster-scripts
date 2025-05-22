// Fill out your copyright notice in the Description page of Project Settings.

#include "ScoreboardWidget.h"
#include "ScoreboardRowWidget.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UScoreboardWidget::Setup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
}

void UScoreboardWidget::Teardown()
{
	RemoveFromParent();
}

void UScoreboardWidget::UpdateScoreboard()
{
	SetTextMinWidths();
	UpdateGameModeText();
	UpdateScoreboardRows();
}

void UScoreboardWidget::SetTextMinWidths()
{
	if (PlayerNameText && CapturesText && !bInitialMinWidthsSet)
	{
		PlayerNameTextMinWidth = PlayerNameText->GetMinDesiredWidth();
		CapturesTextMinWidth = CapturesText->GetMinDesiredWidth();

		bInitialMinWidthsSet = true;
	}
}

void UScoreboardWidget::UpdateGameModeText()
{
	if (GameModeText == nullptr)
	{
		return;
	}

	if (const ABlasterGameState* GameState = GetWorld()->GetGameState<ABlasterGameState>())
	{
		bIsCaptureMode = GameState->IsCaptureMode();

		const FString GameModeName = GameState->GetGameModeName();
		const int32 CurrentPlayers = GameState->PlayerArray.Num();
		const int32 MaxPlayers = GameState->GetMaxPlayers();

		const FString FormattedText = FString::Printf(TEXT("%s (%d/%d)"), *GameModeName, CurrentPlayers, MaxPlayers);
		GameModeText->SetText(FText::FromString(FormattedText));
	}
}

void UScoreboardWidget::UpdateScoreboardRows()
{
	ShowCapturesText(bIsCaptureMode);

	if (ScoreboardRows == nullptr)
	{
		return;
	}

	ScoreboardRows->ClearChildren();

	const ABlasterGameState* GameState = GetWorld()->GetGameState<ABlasterGameState>();
	if (GameState == nullptr)
	{
		return;
	}

	PlayerController = PlayerController == nullptr ? GetWorld()->GetFirstPlayerController() : PlayerController.Get();
	const ABlasterPlayerState* LocalPlayerState = PlayerController
		                                              ? PlayerController->GetPlayerState<ABlasterPlayerState>()
		                                              : nullptr;
	const ETeam LocalTeam = LocalPlayerState ? LocalPlayerState->GetTeam() : ETeam::ET_NoTeam;
	TArray<ABlasterPlayerState*> SortedPlayerStates = GameState->GetSortedPlayerStates(LocalTeam);

	// Create rows for each player
	for (int32 i = 0; i < SortedPlayerStates.Num(); i++)
	{
		ABlasterPlayerState* PlayerState = SortedPlayerStates[i];
		if (PlayerState == nullptr)
		{
			continue;
		}

		if (UScoreboardRowWidget* Row = CreateWidget<UScoreboardRowWidget>(this, ScoreboardRowClass))
		{
			// Fill rows with player info
			const FString PlayerName = PlayerState->GetPlayerName();
			const int32 Captures = PlayerState->GetCaptures();
			const int32 Eliminations = PlayerState->GetEliminations();
			const int32 Assists = PlayerState->GetAssists();
			const int32 Defeats = PlayerState->GetDefeats();
			const int32 Ping = FMath::RoundToInt(PlayerState->GetPingInMilliseconds());
			const ETeam Team = PlayerState->GetTeam();
			const bool bIsLocalPlayer = PlayerController && PlayerController->PlayerState == PlayerState;

			// Use row index for top player rank, but only for players with at least one elimination
			const int32 TopPlayerRank = i < 3 && PlayerState->GetEliminations() > 0 ? i : -1;

			Row->ShowCapturesText(bIsCaptureMode, PlayerNameTextMinWidth, CapturesTextMinWidth);
			Row->SetPlayerInfo(PlayerName, Captures, Eliminations, Assists, Defeats, Ping, Team, bIsLocalPlayer,
			                   TopPlayerRank);

			ScoreboardRows->AddChild(Row);
		}
	}

	// Add empty rows to fill scoreboard
	for (int32 i = SortedPlayerStates.Num(); i < MaxScoreboardRows; i++)
	{
		if (UScoreboardRowWidget* EmptyRow = CreateWidget<UScoreboardRowWidget>(this, ScoreboardRowClass))
		{
			EmptyRow->ShowCapturesText(bIsCaptureMode, PlayerNameTextMinWidth, CapturesTextMinWidth);

			ScoreboardRows->AddChild(EmptyRow);
		}
	}
}

void UScoreboardWidget::ShowCapturesText(const bool bShow)
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
