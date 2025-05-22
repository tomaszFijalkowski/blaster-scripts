// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/PlayerStart.h"
#include "TeamPlayerStart.generated.h"

UCLASS()
class BLASTER_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	ATeamPlayerStart(const FObjectInitializer& ObjectInitializer);

private:
	UPROPERTY(EditAnywhere)
	ETeam Team;

	UPROPERTY(EditAnywhere)
	bool bIsFallbackSpawn;

public:
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE bool IsFallbackSpawn() const { return bIsFallbackSpawn; }
};
