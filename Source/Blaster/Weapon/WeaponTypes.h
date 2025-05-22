#pragma once

#define TRACE_LENGTH 80000.f

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(Display = "Assault Rifle"),
	EWT_RocketLauncher UMETA(Display = "Rocket Launcher"),
	EWT_Pistol UMETA(Display = "Pistol"),
	EWT_SMG UMETA(Display = "SMG"),
	EWT_Shotgun UMETA(Display = "Shotgun"),
	EWT_SniperRifle UMETA(Display = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(Display = "Grenade Launcher"),
	EWT_Flag UMETA(Display = "Flag"),
	EWT_MAX UMETA(Display = "Default Max")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "Default Max")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_None UMETA(DisplayName = "None"),
	EFT_Hitscan UMETA(DisplayName = "Hitscan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	EFT_MAX UMETA(DisplayName = "Default Max")
};
