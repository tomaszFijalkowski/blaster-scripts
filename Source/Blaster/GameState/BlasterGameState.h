// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

enum class ETeam : uint8;
class ABlasterPlayerState;

UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopPlayers(ABlasterPlayerState* Player);
	void RemoveTopPlayer(ABlasterPlayerState* Player);
	ABlasterPlayerState* GetMVP();

	void AddToBlueTeam(ABlasterPlayerState* PlayerState);
	void AddToRedTeam(ABlasterPlayerState* PlayerState);
	void RemoveFromBlueTeam(ABlasterPlayerState* PlayerState);
	void RemoveFromRedTeam(ABlasterPlayerState* PlayerState);

	void IncreaseBlueTeamScore();
	void IncreaseRedTeamScore();

	TArray<ABlasterPlayerState*> GetPlayerStates() const;
	TArray<ABlasterPlayerState*> GetSortedPlayerStates(ETeam LocalTeam) const;

private:
	UPROPERTY(Replicated)
	FString GameModeName;

	UPROPERTY(Replicated)
	bool bIsCaptureMode;

	UPROPERTY(Replicated)
	int32 MaxPlayers;

	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopPlayers;

	TArray<ABlasterPlayerState*> BlueTeam;
	TArray<ABlasterPlayerState*> RedTeam;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	UFUNCTION()
	void OnRep_BlueTeamScore();

	UFUNCTION()
	void OnRep_RedTeamScore();

public:
	FORCEINLINE FString GetGameModeName() const { return GameModeName; }
	FORCEINLINE void SetGameModeName(const FString& Name) { GameModeName = Name; }
	FORCEINLINE bool IsCaptureMode() const { return bIsCaptureMode; }
	FORCEINLINE void SetCaptureMode(const bool bCaptureMode) { bIsCaptureMode = bCaptureMode; }
	FORCEINLINE int32 GetMaxPlayers() const { return MaxPlayers; }
	FORCEINLINE void SetMaxPlayers(const int32 Players) { MaxPlayers = Players; }
	FORCEINLINE const TArray<ABlasterPlayerState*>& GetTopPlayers() const { return TopPlayers; }
	FORCEINLINE const TArray<ABlasterPlayerState*>& GetBlueTeam() const { return BlueTeam; }
	FORCEINLINE const TArray<ABlasterPlayerState*>& GetRedTeam() const { return RedTeam; }
	FORCEINLINE float GetBlueTeamScore() const { return BlueTeamScore; }
	FORCEINLINE float GetRedTeamScore() const { return RedTeamScore; }
};
