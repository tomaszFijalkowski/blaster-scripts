#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(Display = "Unoccupied"),
	ECS_Reloading UMETA(Display = "Reloading"),
	ECS_ThrowingGrenade UMETA(Display = "Throwing Grenade"),
	ECS_SwitchingWeapons UMETA(Display = "Switching Weapons"),
	ECS_MAX UMETA(Display = "Default Max")
};
