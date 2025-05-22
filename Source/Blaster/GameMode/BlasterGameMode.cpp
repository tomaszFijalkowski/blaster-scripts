// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerStart/SpectatorPlayerStart.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController,
                                        ABlasterPlayerController* AttackerController)
{
	if (EliminatedCharacter == nullptr)
	{
		return;
	}

	const ABlasterPlayerController* EliminatingController = EliminatedCharacter->GetMostRecentDamager();

	ABlasterPlayerState* EliminatingPlayer = EliminatingController
		                                         ? Cast<ABlasterPlayerState>(EliminatingController->PlayerState)
		                                         : nullptr;

	const ABlasterPlayerController* AssistingController = EliminatedCharacter->GetSecondMostRecentDamager();

	ABlasterPlayerState* AssistingPlayer = AssistingController
		                                       ? Cast<ABlasterPlayerState>(AssistingController->PlayerState)
		                                       : nullptr;

	ABlasterPlayerState* Victim = VictimController
		                              ? Cast<ABlasterPlayerState>(VictimController->PlayerState)
		                              : nullptr;

	if (EliminatingPlayer)
	{
		EliminatingPlayer->AddToEliminations(1);

		if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
		{
			BlasterGameState->UpdateTopPlayers(EliminatingPlayer);
		}
	}

	if (AssistingPlayer)
	{
		AssistingPlayer->AddToAssists(1);
	}

	if (Victim)
	{
		Victim->AddToDefeats(1);
	}

	if (!EliminatedCharacter->IsEliminated())
	{
		EliminatedCharacter->Eliminate();

		for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*Iterator);
			if (BlasterPlayerController && Victim)
			{
				const bool bSelfElimination = AttackerController == VictimController;
				BlasterPlayerController->BroadcastElimination(
					EliminatingPlayer,
					AssistingPlayer,
					Victim,
					bSelfElimination
				);
			}
		}
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if (EliminatedController)
	{
		RestartPlayer(EliminatedController);
	}
}

void ABlasterGameMode::Logout(AController* Exiting)
{
	if (Exiting == nullptr)
	{
		return;
	}

	if (ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(Exiting->PlayerState))
	{
		PlayerLeavingGame(BlasterPlayerState);
	}

	Super::Logout(Exiting);
}

float ABlasterGameMode::CalculateDamage(AController* Attacker, AController* Victim, const float Damage) const
{
	return Damage;
}

// Used for the initial spawn when joining a match
AActor* ABlasterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (GetMatchState() == MatchState::InProgress)
	{
		return DeterminePlayerStart(Player);
	}

	return DetermineSpectatorPlayerStart();
}

// Used when a player needs to be respawned during gameplay
AActor* ABlasterGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	if (GetMatchState() == MatchState::InProgress)
	{
		return DeterminePlayerStart(Player);
	}

	return DetermineSpectatorPlayerStart();
}

void ABlasterGameMode::PlayerLeavingGame(ABlasterPlayerState* LeavingPlayerState)
{
	if (LeavingPlayerState == nullptr)
	{
		return;
	}

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && BlasterGameState->GetTopPlayers().Contains(LeavingPlayerState))
	{
		BlasterGameState->RemoveTopPlayer(LeavingPlayerState);
	}

	if (ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(LeavingPlayerState->GetPawn()))
	{
		LeavingCharacter->LeaveGame();
	}
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();

	if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
	{
		BlasterGameState->SetGameModeName(GameModeName);
		BlasterGameState->SetCaptureMode(bIsCaptureMode);

		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UMultiplayerSessionsSubsystem* Subsystem =
				GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>())
			{
				const int32 MaxPublicConnections = Subsystem->GetDesiredNumPublicConnections();
				BlasterGameState->SetMaxPlayers(MaxPublicConnections);
			}
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*Iterator))
		{
			BlasterPlayerController->OnMatchStateSet(MatchState, bIsTeamsMatch);
		}
	}
}

void ABlasterGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	GetWorldTimerManager().SetTimer(WarmupTimer, this, &AGameMode::StartMatch, WarmupTime, false);
}

void ABlasterGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	GetWorldTimerManager().SetTimer(MatchTimer, this, &ABlasterGameMode::OnStartCooldown, MatchTime, false);
}

void ABlasterGameMode::OnStartCooldown()
{
	SetMatchState(MatchState::Cooldown);

	GetWorldTimerManager().SetTimer(CooldownTimer, this, &AGameMode::RestartGame, CooldownTime, false);
}

AActor* ABlasterGameMode::DeterminePlayerStart(const AController* Controller)
{
	const ABlasterPlayerState* BlasterPlayerState = Controller->GetPlayerState<ABlasterPlayerState>();
	const ETeam PlayerTeam = BlasterPlayerState ? BlasterPlayerState->GetTeam() : ETeam::ET_NoTeam;

	bool bShouldUseFallbackSpawn = false;
	if (const ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(Controller))
	{
		bShouldUseFallbackSpawn = BlasterPlayerController->ShouldUseFallbackSpawn();
	}

	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStartActors);

	TArray<ATeamPlayerStart*> PlayerStarts;
	for (const auto PlayerStartActor : PlayerStartActors)
	{
		ATeamPlayerStart* PlayerStart = Cast<ATeamPlayerStart>(PlayerStartActor);
		if (PlayerStart && PlayerStart->GetTeam() == PlayerTeam &&
			PlayerStart->IsFallbackSpawn() == bShouldUseFallbackSpawn)
		{
			PlayerStarts.Add(PlayerStart);
		}
	}

	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), PlayerActors);

	TArray<AActor*> Players;
	for (const auto PlayerActor : PlayerActors)
	{
		if (ABlasterCharacter* Player = Cast<ABlasterCharacter>(PlayerActor))
		{
			Players.Add(Player);
		}
	}

	TArray<TTuple<float, AActor*>> FarthestPlayers = {};

	for (auto PlayerStart : PlayerStarts)
	{
		float MinDistance = TNumericLimits<float>::Max();

		for (const auto Player : Players)
		{
			const float Distance = FVector::Dist(PlayerStart->GetActorLocation(), Player->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
			}
		}

		FarthestPlayers.Add({MinDistance, PlayerStart});
	}

	FarthestPlayers.Sort([](const TTuple<float, AActor*>& A, const TTuple<float, AActor*>& B)
	{
		return A.Get<0>() > B.Get<0>();
	});

	// Calculate the selection pool size based on the number of players
	const int32 SelectionPoolSize = FMath::Max(FarthestPlayers.Num() / 2.f, Players.Num());
	const int32 ClampedPoolSize = FMath::Min(SelectionPoolSize, FarthestPlayers.Num());
	const int32 Selection = FMath::RandRange(0, FMath::Max(0, ClampedPoolSize - 1));

	return FarthestPlayers[Selection].Value;
}

AActor* ABlasterGameMode::DetermineSpectatorPlayerStart()
{
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(this, ASpectatorPlayerStart::StaticClass(), PlayerStartActors);

	if (PlayerStartActors.Num() == 0)
	{
		// Fallback to regular player starts if no spectator starts exist
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStartActors);

		if (PlayerStartActors.Num() == 0)
		{
			return nullptr;
		}
	}

	const int32 Selection = FMath::RandRange(0, FMath::Max(0, PlayerStartActors.Num() - 1));
	return PlayerStartActors[Selection];
}
