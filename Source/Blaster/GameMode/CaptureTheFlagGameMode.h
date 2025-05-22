// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"

class AFlagZone;
class AFlag;

UCLASS()
class BLASTER_API ACaptureTheFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController) override;

	void FlagCaptured(const AFlag* Flag, const AFlagZone* FlagZone);

private:
	ABlasterPlayerState* GetScoringPlayerState(const AFlag* Flag) const;
};
