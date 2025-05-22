// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class BLASTER_API UScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup();
	void Teardown();
	void UpdateScoreboard();

private:
	void SetTextMinWidths();
	void UpdateGameModeText();
	void UpdateScoreboardRows();
	void ShowCapturesText(bool bShow);

	bool bIsCaptureMode = false;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GameModeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	float PlayerNameTextMinWidth = 0.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CapturesText;

	float CapturesTextMinWidth = 0.f;

	bool bInitialMinWidthsSet = false;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ScoreboardRows;

	UPROPERTY(EditAnywhere, Category = "Scoreboard")
	int32 MaxScoreboardRows = 16;

	UPROPERTY(EditAnywhere, Category = "Scoreboard")
	TSubclassOf<UUserWidget> ScoreboardRowClass;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
