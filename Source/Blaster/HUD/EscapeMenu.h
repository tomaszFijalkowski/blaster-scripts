// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeMenu.generated.h"

class USettingsWidget;
class UMultiplayerSessionsSubsystem;
class UButton;

UCLASS()
class BLASTER_API UEscapeMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup();
	void Teardown();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetSettingsPanelOpen(const bool bOpen) { bIsSettingsPanelOpen = bOpen; }

	void CloseSettingsPanel();

protected:
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnPlayerLeaveGame();

private:
	UFUNCTION()
	void LeaveButtonClicked();

	UFUNCTION()
	void QuitButtonClicked();

	void LeaveAndReturnToMainMenu();
	void LeaveAndQuit();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<USettingsWidget> SettingsPanel;

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	bool bLeaveAndQuit = false;

	bool bIsSettingsPanelOpen = false;

public:
	FORCEINLINE bool IsSettingsPanelOpen() const { return bIsSettingsPanelOpen; }
};
