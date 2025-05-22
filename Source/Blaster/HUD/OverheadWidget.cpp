// Fill out your copyright notice in the Description page of Project Settings.

#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ToggleWithFade(const bool bIsVisible)
{
	if (bIsVisible != bIsCurrentlyVisible)
	{
		bIsCurrentlyVisible = bIsVisible;

		if (bIsVisible)
		{
			PlayFadeInAnimation();
		}
		else
		{
			PlayFadeOutAnimation();
		}
	}
}

void UOverheadWidget::ShowPlayerNetRole(const APawn* InPawn)
{
	const ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (RemoteRole)
	{
	case ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ROLE_Authority:
		Role = FString("Authority");
		break;
	case ROLE_None:
		Role = FString("None");
		break;
	default:
		break;
	}

	const FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	SetDisplayText(RemoteRoleString);
}

void UOverheadWidget::ShowPlayerName(const APawn* InPawn)
{
	if (const APlayerState* PlayerState = InPawn->GetPlayerState())
	{
		const FString Name = PlayerState->GetPlayerName();
		SetDisplayText(Name);
	}
}

void UOverheadWidget::PlayFadeInAnimation()
{
	if (IsAnyAnimationPlaying())
	{
		StopAllAnimations();
	}

	if (FadeInAnimation)
	{
		PlayAnimation(FadeInAnimation);
	}
}

void UOverheadWidget::PlayFadeOutAnimation()
{
	if (IsAnyAnimationPlaying())
	{
		StopAllAnimations();
	}

	if (FadeOutAnimation)
	{
		PlayAnimation(FadeOutAnimation);
	}
}
