// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

class ABlasterPlayerState;

UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	void PlayerLeavingGame(const ABlasterPlayerState* LeavingPlayer);

private:
	void BroadcastNumberOfPlayersChanged(int32 NumberOfPlayers, int32 MaxPlayers);
	void StartGame(const UMultiplayerSessionsSubsystem* Subsystem);

	UPROPERTY(EditDefaultsOnly, Category = "Lobby Properties")
	TMap<FString, TSoftObjectPtr<UWorld>> MatchTypeToMap;

	UPROPERTY(EditAnywhere, Category = "Lobby Properties")
	float MatchStartingDelay = 3.f;
};
