// Fill out your copyright notice in the Description page of Project Settings.

#include "TeamsGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

ATeamsGameMode::ATeamsGameMode()
{
	bIsTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
	if (BlasterGameState && BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
	{
		AssignPlayerToTeam(BlasterGameState, BlasterPlayerState);
	}

	// Remove the player from the inactive array as to not override with the old player state without the team
	if (BlasterPlayerState)
	{
		const FUniqueNetIdRepl& NewPlayerUniqueId = BlasterPlayerState->GetUniqueId();
		InactivePlayerArray.RemoveAll([&NewPlayerUniqueId](const APlayerState* InactiveState)
		{
			return IsValid(InactiveState) && InactiveState->GetUniqueId() == NewPlayerUniqueId;
		});
	}

	Super::PostLogin(NewPlayer);
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (BlasterGameState && BlasterPlayerState)
	{
		RemovePlayerFromTeam(BlasterGameState, BlasterPlayerState);
	}

	Super::Logout(Exiting);
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, const float Damage) const
{
	if (Attacker == nullptr || Victim == nullptr)
	{
		return Damage;
	}

	const ABlasterPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ABlasterPlayerState>();
	const ABlasterPlayerState* VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();

	const bool bInvalidPlayerState = AttackerPlayerState == nullptr || VictimPlayerState == nullptr;
	const bool bSamePlayer = AttackerPlayerState == VictimPlayerState;
	if (bInvalidPlayerState || bSamePlayer)
	{
		return Damage;
	}

	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		return 0.f;
	}

	return Damage;
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                      ABlasterPlayerController* VictimController,
                                      ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);

	// Do not score if victim and attacker are the same player
	if (VictimController == AttackerController)
	{
		return;
	}

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	const ABlasterPlayerState* AttackerPlayerState =
		AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	if (BlasterGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->IncreaseBlueTeamScore();
		}
		else if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->IncreaseRedTeamScore();
		}
	}
}

void ATeamsGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		DisconnectionCheckTimer,
		this,
		&ATeamsGameMode::CheckForDisconnectedPlayers,
		DisconnectionCheckTimerInterval,
		true
	);
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
	{
		for (const auto PlayerState : BlasterGameState->GetPlayerStates())
		{
			AssignPlayerToTeam(BlasterGameState, PlayerState);
		}
	}

	Super::HandleMatchHasStarted();
}

void ATeamsGameMode::AssignPlayerToTeam(ABlasterGameState* BlasterGameState, ABlasterPlayerState* BlasterPlayerState)
{
	check(BlasterGameState);
	check(BlasterPlayerState);

	RemovePlayerFromTeam(BlasterGameState, BlasterPlayerState);

	if (BlasterGameState->GetRedTeam().Num() >= BlasterGameState->GetBlueTeam().Num())
	{
		BlasterGameState->AddToBlueTeam(BlasterPlayerState);
		BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
	}
	else
	{
		BlasterGameState->AddToRedTeam(BlasterPlayerState);
		BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
	}
}

void ATeamsGameMode::RemovePlayerFromTeam(ABlasterGameState* BlasterGameState, ABlasterPlayerState* BlasterPlayerState)
{
	check(BlasterGameState)
	check(BlasterPlayerState)

	if (BlasterGameState->GetBlueTeam().Contains(BlasterPlayerState))
	{
		BlasterGameState->RemoveFromBlueTeam(BlasterPlayerState);
	}
	else if (BlasterGameState->GetRedTeam().Contains(BlasterPlayerState))
	{
		BlasterGameState->RemoveFromRedTeam(BlasterPlayerState);
	}
}

void ATeamsGameMode::CheckForDisconnectedPlayers()
{
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState == nullptr)
	{
		return;
	}

	for (auto PreviousPlayerState : PreviousPlayerStates)
	{
		if (!BlasterGameState->GetPlayerStates().Contains(PreviousPlayerState))
		{
			RemovePlayerFromTeam(BlasterGameState, PreviousPlayerState);
		}
	}

	PreviousPlayerStates = BlasterGameState->GetPlayerStates();
}
