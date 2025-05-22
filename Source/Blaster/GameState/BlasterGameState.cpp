// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Net/UnrealNetwork.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, GameModeName);
	DOREPLIFETIME(ABlasterGameState, bIsCaptureMode);
	DOREPLIFETIME(ABlasterGameState, MaxPlayers);
	DOREPLIFETIME(ABlasterGameState, TopPlayers);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
}

void ABlasterGameState::UpdateTopPlayers(ABlasterPlayerState* Player)
{
	ABlasterCharacter* PreviousMVP = TopPlayers.Num() > 0 ? Cast<ABlasterCharacter>(TopPlayers[0]->GetPawn()) : nullptr;

	TopPlayers.AddUnique(Player);
	TopPlayers.Sort([](const ABlasterPlayerState& A, const ABlasterPlayerState& B)
	{
		if (A.GetEliminations() == B.GetEliminations())
		{
			return A.GetEliminationTimestamp() < B.GetEliminationTimestamp();
		}
		return A.GetEliminations() > B.GetEliminations();
	});

	ABlasterCharacter* CurrentMVP = TopPlayers.Num() > 0 ? Cast<ABlasterCharacter>(TopPlayers[0]->GetPawn()) : nullptr;

	if (CurrentMVP != nullptr && CurrentMVP != PreviousMVP)
	{
		CurrentMVP->MulticastGainedMVP();

		if (PreviousMVP != nullptr)
		{
			PreviousMVP->MulticastLostMVP();
		}
	}
}

void ABlasterGameState::RemoveTopPlayer(ABlasterPlayerState* Player)
{
	TopPlayers.Remove(Player);
}

ABlasterPlayerState* ABlasterGameState::GetMVP()
{
	if (TopPlayers.Num() > 0)
	{
		return TopPlayers[0];
	}

	return nullptr;
}

void ABlasterGameState::AddToBlueTeam(ABlasterPlayerState* PlayerState)
{
	BlueTeam.AddUnique(PlayerState);
}

void ABlasterGameState::AddToRedTeam(ABlasterPlayerState* PlayerState)
{
	RedTeam.AddUnique(PlayerState);
}

void ABlasterGameState::RemoveFromBlueTeam(ABlasterPlayerState* PlayerState)
{
	BlueTeam.Remove(PlayerState);
}

void ABlasterGameState::RemoveFromRedTeam(ABlasterPlayerState* PlayerState)
{
	RedTeam.Remove(PlayerState);
}

void ABlasterGameState::IncreaseBlueTeamScore()
{
	BlueTeamScore++;

	if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(
		GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::IncreaseRedTeamScore()
{
	RedTeamScore++;

	if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(
		GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}

TArray<ABlasterPlayerState*> ABlasterGameState::GetPlayerStates() const
{
	TArray<ABlasterPlayerState*> PlayerStates;

	for (auto PlayerState : PlayerArray)
	{
		if (ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState))
		{
			PlayerStates.Add(BlasterPlayerState);
		}
	}

	return PlayerStates;
}

TArray<ABlasterPlayerState*> ABlasterGameState::GetSortedPlayerStates(ETeam LocalTeam) const
{
	TArray<ABlasterPlayerState*> SortedPlayers = GetPlayerStates();

	SortedPlayers.Sort([LocalTeam](const ABlasterPlayerState& A, const ABlasterPlayerState& B)
	{
		// First sort by team (local team first)
		if (A.GetTeam() != B.GetTeam())
		{
			return LocalTeam == ETeam::ET_BlueTeam ? A.GetTeam() < B.GetTeam() : A.GetTeam() > B.GetTeam();
		}

		// Then by eliminations
		if (A.GetEliminations() != B.GetEliminations())
		{
			return A.GetEliminations() > B.GetEliminations();
		}

		// Finally by elimination timestamp
		return A.GetEliminationTimestamp() < B.GetEliminationTimestamp();
	});

	return SortedPlayers;
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(
		GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(
		GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->SetHUDRedTeamScore(RedTeamScore);
	}
}
