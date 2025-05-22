#pragma once

UENUM(BlueprintType)
enum class EBuffType : uint8
{
	EBT_Shield UMETA(Display = "Shield"),
	EBT_Speed UMETA(Display = "Speed"),
	EBT_Jump UMETA(Display = "Jump"),
	EBT_Damage UMETA(Display = "Damage"),
	EBT_MAX UMETA(Display = "Default Max")
};
