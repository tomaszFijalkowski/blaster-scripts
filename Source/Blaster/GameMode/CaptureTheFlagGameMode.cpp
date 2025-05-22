// Fill out your copyright notice in the Description page of Project Settings.

#include "CaptureTheFlagGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/Zone/FlagZone.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                               ABlasterPlayerController* VictimController,
                                               ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(const AFlag* Flag, const AFlagZone* FlagZone)
{
	if (Flag->GetTeam() != FlagZone->GetTeam())
	{
		if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
		{
			if (FlagZone->GetTeam() == ETeam::ET_BlueTeam)
			{
				BlasterGameState->IncreaseBlueTeamScore();
			}
			else if (FlagZone->GetTeam() == ETeam::ET_RedTeam)
			{
				BlasterGameState->IncreaseRedTeamScore();
			}
		}

		ABlasterPlayerState* ScoringPlayer = GetScoringPlayerState(Flag);

		if (ScoringPlayer)
		{
			ScoringPlayer->AddToCaptures(1);
		}

		for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*Iterator);
			if (BlasterPlayerController && ScoringPlayer)
			{
				BlasterPlayerController->BroadcastFlagCapture(ScoringPlayer);
			}
		}
	}
}

ABlasterPlayerState* ACaptureTheFlagGameMode::GetScoringPlayerState(const AFlag* Flag) const
{
	if (AActor* OwnerActor = Flag->GetOwner())
	{
		if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
		{
			if (const AController* OwnerController = OwnerPawn->GetController())
			{
				return OwnerController->GetPlayerState<ABlasterPlayerState>();
			}
		}
	}
	return nullptr;
}
