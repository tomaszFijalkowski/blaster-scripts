// Fill out your copyright notice in the Description page of Project Settings.

#include "WaitingForPlayersWidget.h"

void UWaitingForPlayersWidget::PlayMatchStartingAnimation()
{
	if (MatchStartingAnimation)
	{
		PlayAnimation(MatchStartingAnimation);
	}
}
