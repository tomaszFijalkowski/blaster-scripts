// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "BlasterDamageType.generated.h"

UCLASS()
class BLASTER_API UBlasterDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	virtual bool IsHeadshot() const { return false; }
	virtual bool IsAoeDamage() const { return false; }
};
