// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterPlayerState;

namespace MatchState
{
	// Match duration has been reached. Display end screen and begin cooldown timer
	extern BLASTER_API const FName Cooldown;
}

class ABlasterPlayerController;
class ABlasterCharacter;

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController,
	                              ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float Damage) const;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = L"") override;
	void PlayerLeavingGame(ABlasterPlayerState* LeavingPlayerState);

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	void OnStartCooldown();

	bool bIsTeamsMatch = false;

private:
	AActor* DeterminePlayerStart(const AController* Controller);
	AActor* DetermineSpectatorPlayerStart();

	UPROPERTY(EditDefaultsOnly, Category = "Game Mode Settings")
	FString GameModeName = FString();

	UPROPERTY(EditDefaultsOnly, Category = "Game Mode Settings")
	bool bIsCaptureMode = false;

	UPROPERTY(EditDefaultsOnly, Category = "Game Mode Settings")
	float WarmupTime = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Game Mode Settings")
	float MatchTime = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Game Mode Settings")
	float CooldownTime = 10.f;

	FTimerHandle WarmupTimer;
	FTimerHandle MatchTimer;
	FTimerHandle CooldownTimer;

	float LevelStartingTime = 0.f;

public:
	FORCEINLINE const FString& GetGameModeName() const { return GameModeName; }
	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE float GetMatchTime() const { return MatchTime; }
	FORCEINLINE float GetCooldownTime() const { return CooldownTime; }
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }
	FORCEINLINE bool IsTeamsMatch() const { return bIsTeamsMatch; }
};
