#pragma once

namespace Socket
{
	const FName LeftHandSocket = TEXT("LeftHandSocket");
	const FName RightHandSocket = TEXT("RightHandSocket");
	const FName GrenadeSocket = TEXT("GrenadeSocket");
	const FName FlagSocket = TEXT("FlagSocket");
	const FName BackpackSocket = TEXT("BackpackSocket");
	const FName RocketLauncherBackpackSocket = TEXT("RocketLauncherBackpackSocket");
	const FName SMGBackpackSocket = TEXT("SMGBackpackSocket");
	const FName ShotgunBackpackSocket = TEXT("ShotgunBackpackSocket");
	const FName AmmoEject = TEXT("AmmoEject");
	const FName MuzzleFlash = TEXT("MuzzleFlash");
	const FName PickupSocket = TEXT("PickupSocket");
}

namespace Bone
{
	const FName HeadBone = TEXT("head");
	const FName PelvisBone = TEXT("pelvis");
	const FName Spine1Bone = TEXT("spine_01");
	const FName Spine2Bone = TEXT("spine_02");
	const FName Spine3Bone = TEXT("spine_03");
	const FName LeftUpperArmBone = TEXT("upperarm_l");
	const FName RightUpperArmBone = TEXT("upperarm_r");
	const FName LeftLowerArmBone = TEXT("lowerarm_l");
	const FName RightLowerArmBone = TEXT("lowerarm_r");
	const FName LeftHandBone = TEXT("hand_l");
	const FName RightHandBone = TEXT("hand_r");
	const FName LeftThighBone = TEXT("thigh_l");
	const FName RightThighBone = TEXT("thigh_r");
	const FName LeftCalfBone = TEXT("calf_l");
	const FName RightCalfBone = TEXT("calf_r");
	const FName LeftFootBone = TEXT("foot_l");
	const FName RightFootBone = TEXT("foot_r");
	const FName BackpackBone = TEXT("backpack");
	const FName RootBone = TEXT("root");
}

namespace MontageSection
{
	// Stances
	const FName RifleAim = FName("RifleAim");
	const FName RifleHip = FName("RifleHip");

	// Weapon types
	const FName AssaultRifle = FName("AssaultRifle");
	const FName RocketLauncher = FName("RocketLauncher");
	const FName Pistol = FName("Pistol");
	const FName SMG = FName("SMG");
	const FName Shotgun = FName("Shotgun");
	const FName SniperRifle = FName("SniperRifle");
	const FName GrenadeLauncher = FName("GrenadeLauncher");
	const FName Default = FName("Default");

	// Hit reactions
	const FName HitReact1 = FName("HitReact1");
	const FName HitReact2 = FName("HitReact2");
}

namespace MaterialParameter
{
	// Dissolve material
	const FName Dissolve = FName("Dissolve");
	const FName Glow = FName("Glow");
}

namespace ParticleSystemParameter
{
	const FName Target = FName("Target");
}

namespace Announcement
{
	const FName MatchStart = FName("Match starts in: ");
	const FName NewMatchStart = FName("New match starts in: ");
}
