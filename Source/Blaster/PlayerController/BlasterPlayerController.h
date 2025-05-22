// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

#define COUNTDOWN_BLINKING_THRESHOLD 11.f

class UScoreboardWidget;
class ABlasterPlayerState;
class UEscapeMenu;
class UInputAction;
struct FInputActionValue;
class UInputMappingContext;
class UAnnouncementWidget;
class ABlasterHUD;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PawnLeavingGame() override;
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible
	virtual float GetServerTime(); // Synced with server world clock
	void SetPostProcessingEnabled(bool bEnabled);
	void SetHUDHealth(float Health, float MaxHealth, float HealthBarPercent);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDRespawnBoxVisibility(const bool bVisibility);
	void SetHUDRespawnTime(const float RespawnTime);
	void SetHUDWeaponAmmo(int32 Ammo, FLinearColor Color = FLinearColor::Black);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(const float CountdownTime);
	void SetHUDAnnouncementCountdown(const float CountdownTime);
	void SetHUDGrenades(const int32 Grenades);
	void SetHUDTeamScoreVisibility() const;
	void SetHUDBlueTeamScore(const int32 Score);
	void SetHUDRedTeamScore(const int32 Score);
	void SetHUDReloadProgress(float ReloadProgress, float ReloadDuration);
	void SetHUDReloadProgressVisibility(const bool bVisibility);
	void SetHUDBuffDurationBars(TArray<FBuffDurationEntry> BuffDurationEntries);
	void OnMatchStateSet(FName State, bool bInIsTeamsMatch = false);
	void BroadcastElimination(APlayerState* EliminatingPlayer, APlayerState* AssistingPlayer, APlayerState* Victim,
	                          bool bSelfElimination);
	void BroadcastFlagCapture(ABlasterPlayerState* ScoringPlayer);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void Escape(const FInputActionValue& Value);
	void Tab(const FInputActionValue& Value);

	void PollInitCharacterOverlay();
	void PollInitTeamScore();

	void SetHUDTime();

	void HandleMatchHasStarted(bool bInIsTeamsMatch = false);
	void HandleMatchCooldown();

	UFUNCTION(Client, Reliable)
	void ClientElimination(APlayerState* EliminatingPlayer, APlayerState* AssistingPlayer, APlayerState* Victim,
	                       const bool bSelfElimination);

	UFUNCTION(Client, Reliable)
	void ClientFlagCapture(ABlasterPlayerState* ScoringPlayer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UIMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> EscapeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> TabAction;

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOfServerReceivedClientRequest);

	float SingleTripTime = 0.f;

	// Difference between client and server time
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName InMatchState, float InWarmupTime, float InMatchTime, float InCooldownTime,
	                       float InLevelStartingTime, bool bInIsTeamsMatch = false);

	void SetHUDPingAmount(float PingAmount);
	void EnableHUDHighImage(bool bEnable);

private:
	void InitializeEnhancedInput() const;
	void CheckPing(float DeltaTime);
	void CheckTimeSync(float DeltaTime);
	void ShowTopPlayers(const TObjectPtr<UAnnouncementWidget>& Announcement);
	void ShowWinningTeam(const TObjectPtr<UAnnouncementWidget>& Announcement);
	TObjectPtr<UCharacterOverlay> GetValidCharacterOverlay();
	TObjectPtr<UAnnouncementWidget> GetValidAnnouncement();

	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

	float LevelStartingTime = 0.f;
	float WarmupTime = 0.f;
	float MatchTime = 0.f;
	float CooldownTime = 0.f;

	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;

	bool bIsCharacterOverlayInitialized = false;
	bool bIsTeamScoreInitialized = false;

	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;

	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;

	int32 HUDGrenades;
	bool bInitializeGrenades = false;

	int32 HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	int32 HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;

	bool bIsPingDisplayed = false;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "High Ping Properties")
	float CheckPingFrequency = 0.5f;

	UPROPERTY(EditAnywhere, Category = "High Ping Properties")
	float HighPingThreshold = 100.f;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> EscapeMenuClass;

	UPROPERTY()
	TObjectPtr<UEscapeMenu> EscapeMenu;

	bool bIsEscapeMenuOpen = false;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> ScoreboardWidgetClass;

	UPROPERTY()
	TObjectPtr<UScoreboardWidget> ScoreboardWidget;

	UPROPERTY(Replicated)
	bool bIsTeamsMatch = false;

	void UpdateBuffDurationBar(UProgressBar* BuffDurationBar, const FBuffDurationEntry& BuffDurationEntry);
	void CollapseRemainingBuffDurationBars(int32 BuffDurationEntriesNum);

	UPROPERTY(EditAnywhere, Category = "UI")
	float ScoreboardUpdateInterval = 0.2f;

	FTimerHandle ScoreboardUpdateTimer;

	void StartScoreboardUpdates();
	void StopScoreboardUpdates();

	UFUNCTION()
	void UpdateScoreboard();

	bool bShouldUseFallbackSpawn = false;

public:
	float GetSingleTripTime() const { return SingleTripTime; }
	bool GetIsEscapeMenuOpen() const { return bIsEscapeMenuOpen; }
	bool ShouldUseFallbackSpawn() const { return bShouldUseFallbackSpawn; }
	void SetShouldUseFallbackSpawn(const bool bShouldUse) { bShouldUseFallbackSpawn = bShouldUse; }
};
