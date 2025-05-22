// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterDamageType.h"
#include "GameFramework/DamageType.h"
#include "AoeDamageType.generated.h"

UCLASS()
class BLASTER_API UAoeDamageType : public UBlasterDamageType
{
	GENERATED_BODY()

public:
	virtual bool IsHeadshot() const override { return false; }
	virtual bool IsAoeDamage() const override { return true; }
};
