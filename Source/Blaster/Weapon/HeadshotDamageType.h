// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterDamageType.h"
#include "GameFramework/DamageType.h"
#include "HeadshotDamageType.generated.h"

UCLASS()
class BLASTER_API UHeadshotDamageType : public UBlasterDamageType
{
	GENERATED_BODY()

public:
	virtual bool IsHeadshot() const override { return true; }
	virtual bool IsAoeDamage() const override { return false; }
};
