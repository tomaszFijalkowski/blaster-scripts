// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "BlasterSpectatorPawn.generated.h"

UCLASS()
class BLASTER_API ABlasterSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	virtual void PossessedBy(AController* NewController) override;

private:
	void SetInitialRotation();
};
