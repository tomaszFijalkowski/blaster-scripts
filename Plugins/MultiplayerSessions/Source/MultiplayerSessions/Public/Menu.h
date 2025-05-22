// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UTextBlock;
class USlider;
class UMultiplayerSessionsSubsystem;
class UButton;

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(TSoftObjectPtr<UWorld> LobbyLevel, int32 NumberOfPublicConnections = 4);

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	/**
	 * Callbacks for the custom delegates on the MultiplayerSessionSubsystem.
	 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NumberOfPlayersText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> NumberOfPlayersSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DeathmatchButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TeamDeathmatchButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CaptureTheFlagButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

	UFUNCTION()
	void OnNumberOfPlayersChanged(float Value);

	UFUNCTION()
	void OnDeathmatchButtonClicked();

	UFUNCTION()
	void OnTeamDeathmatchButtonClicked();

	UFUNCTION()
	void OnCaptureTheFlagButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	void SetPlayerInputMode();
	void SetMultiplayerSessionSubsystem();
	void MenuTeardown();

	void Log(FColor DisplayColor, const FString& DebugMessage);

	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	FString LobbyPath = TEXT("");
	FString MatchType = TEXT("");
	int32 NumPublicConnections = 4;

	bool bEnableLogging = true;
};
