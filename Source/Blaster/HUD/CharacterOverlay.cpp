// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"

void UCharacterOverlay::PlayMatchCountdownAnimation()
{
	if (MatchCountdownAnimation && !IsAnimationPlaying(MatchCountdownAnimation))
	{
		PlayAnimation(MatchCountdownAnimation);
		UGameplayStatics::PlaySound2D(this, MatchCountdownAnimationSound, 1.f);
	}
}

void UCharacterOverlay::PlayPingAmountFadeInAnimation()
{
	if (PingAmountFadeInAnimation && !IsAnimationPlaying(PingAmountFadeInAnimation))
	{
		PlayAnimation(PingAmountFadeInAnimation);
	}
}

void UCharacterOverlay::PlayHighPingAnimation()
{
	if (HighPingImageAnimation && !IsAnimationPlaying(HighPingImageAnimation))
	{
		PlayAnimation(HighPingImageAnimation, 0.f, INT32_MAX);
	}
}

void UCharacterOverlay::StopHighPingAnimation()
{
	if (HighPingImageAnimation && IsAnimationPlaying(HighPingImageAnimation))
	{
		StopAnimation(HighPingImageAnimation);
	}
}
