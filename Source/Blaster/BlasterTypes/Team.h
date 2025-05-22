#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_BlueTeam UMETA(Display = "Blue Team"),
	ET_RedTeam UMETA(Display = "Red Team"),
	ET_NoTeam UMETA(Display = "No Team"),
	ET_MAX UMETA(Display = "Default Max")
};

inline FString ToString(const ETeam Team)
{
	switch (Team)
	{
	case ETeam::ET_NoTeam:
		return TEXT("NoTeam");
	case ETeam::ET_RedTeam:
		return TEXT("RedTeam");
	case ETeam::ET_BlueTeam:
		return TEXT("BlueTeam");
	default:
		return TEXT("Unknown");
	}
}
