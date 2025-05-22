// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/LobbyPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMultiplayerSessionsSubsystem* Subsystem =
			GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>())
		{
			const int32 NumberOfPlayers = GetNumPlayers();
			const int32 MaxPlayers = Subsystem->GetDesiredNumPublicConnections();

			BroadcastNumberOfPlayersChanged(NumberOfPlayers, MaxPlayers);

			if (NumberOfPlayers == MaxPlayers)
			{
				FTimerHandle StartGameTimer;
				GetWorldTimerManager().SetTimer(
					StartGameTimer,
					[this, Subsystem] { StartGame(Subsystem); },
					MatchStartingDelay,
					false
				);
			}
		}
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UMultiplayerSessionsSubsystem* Subsystem =
			GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>())
		{
			const int32 NumberOfPlayers = GetNumPlayers();
			const int32 MaxPlayers = Subsystem->GetDesiredNumPublicConnections();

			BroadcastNumberOfPlayersChanged(NumberOfPlayers, MaxPlayers);
		}
	}
}

void ALobbyGameMode::PlayerLeavingGame(const ABlasterPlayerState* LeavingPlayer)
{
	if (LeavingPlayer == nullptr)
	{
		return;
	}

	if (ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(LeavingPlayer->GetPawn()))
	{
		LeavingCharacter->LeaveGame();
	}
}

void ALobbyGameMode::BroadcastNumberOfPlayersChanged(const int32 NumberOfPlayers, const int32 MaxPlayers)
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (ALobbyPlayerController* LobbyPlayerController = Cast<ALobbyPlayerController>(*Iterator))
		{
			if (LobbyPlayerController->IsLocalController())
			{
				LobbyPlayerController->OnNumberOfPlayersChanged(NumberOfPlayers, MaxPlayers);
			}
			else
			{
				LobbyPlayerController->ClientOnNumberOfPlayersChanged(NumberOfPlayers, MaxPlayers);
			}
		}
	}
}

void ALobbyGameMode::StartGame(const UMultiplayerSessionsSubsystem* Subsystem)
{
	if (UWorld* World = GetWorld())
	{
		bUseSeamlessTravel = true;

		const FString MatchType = Subsystem->GetDesiredMatchType();
		if (const TSoftObjectPtr<UWorld>* LevelPath = MatchTypeToMap.Find(MatchType))
		{
			World->ServerTravel(LevelPath->GetLongPackageName().Append("?listen"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unknown match type: %s"), *MatchType);
		}
	}
}
