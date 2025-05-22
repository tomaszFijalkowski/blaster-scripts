// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "SpectatorPlayerStart.generated.h"

UCLASS()
class BLASTER_API ASpectatorPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	ASpectatorPlayerStart(const FObjectInitializer& ObjectInitializer);
};
