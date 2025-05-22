// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "TeamsGameMode.generated.h"

UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float Damage) const override;
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController) override;

protected:
	virtual void BeginPlay() override;
	virtual void HandleMatchHasStarted() override;

private:
	void AssignPlayerToTeam(ABlasterGameState* BlasterGameState, ABlasterPlayerState* BlasterPlayerState);
	void RemovePlayerFromTeam(ABlasterGameState* BlasterGameState, ABlasterPlayerState* BlasterPlayerState);
	void CheckForDisconnectedPlayers();

	FTimerHandle DisconnectionCheckTimer;

	UPROPERTY(EditAnywhere, Category = "Disconnection Check Properties")
	float DisconnectionCheckTimerInterval = 0.5f;

	TArray<ABlasterPlayerState*> PreviousPlayerStates;
};
