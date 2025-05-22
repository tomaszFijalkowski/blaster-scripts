// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlagResetWidget.generated.h"

class UImage;
class UTextBlock;
enum class ETeam : uint8;

UCLASS()
class BLASTER_API UFlagResetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetFlagResetImage(const ETeam Team);
	void SetFlagResetRemainingTime(const float RemainingTime);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> FlagResetImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FlagResetRemainingTime;
};
