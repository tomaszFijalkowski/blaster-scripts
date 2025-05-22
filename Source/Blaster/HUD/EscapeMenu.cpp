// Fill out your copyright notice in the Description page of Project Settings.

#include "EscapeMenu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "SettingsWidget.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetSystemLibrary.h"

void UEscapeMenu::Setup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (const UWorld* World = GetWorld())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController.Get();
		if (PlayerController)
		{
			const FInputModeGameAndUI InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);

			// Set mouse cursor to the center of the screen
			const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
			PlayerController->SetMouseLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
		}
	}

	if (LeaveButton && !LeaveButton->OnClicked.IsBound())
	{
		LeaveButton->OnClicked.AddDynamic(this, &UEscapeMenu::LeaveButtonClicked);
	}

	if (QuitButton && !QuitButton->OnClicked.IsBound())
	{
		QuitButton->OnClicked.AddDynamic(this, &UEscapeMenu::QuitButtonClicked);
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(
				this, &UEscapeMenu::OnDestroySession);
		}
	}

	bLeaveAndQuit = false;
}

void UEscapeMenu::Teardown()
{
	RemoveFromParent();

	if (const UWorld* World = GetWorld())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController.Get();
		if (PlayerController)
		{
			const FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}

	if (LeaveButton && LeaveButton->OnClicked.IsBound())
	{
		LeaveButton->OnClicked.RemoveDynamic(this, &UEscapeMenu::LeaveButtonClicked);
	}

	if (QuitButton && QuitButton->OnClicked.IsBound())
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UEscapeMenu::QuitButtonClicked);
	}

	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(
			this, &UEscapeMenu::OnDestroySession);
	}
}

void UEscapeMenu::CloseSettingsPanel()
{
	if (SettingsPanel && SettingsPanel->OnBackButtonClicked.IsBound())
	{
		SettingsPanel->OnBackButtonClicked.Broadcast();
	}
}

void UEscapeMenu::OnDestroySession(const bool bWasSuccessful)
{
	if (bLeaveAndQuit)
	{
		if (QuitButton && !bWasSuccessful)
		{
			QuitButton->SetIsEnabled(true);
		}

		LeaveAndQuit();
	}
	else
	{
		if (LeaveButton && !bWasSuccessful)
		{
			LeaveButton->SetIsEnabled(true);
		}

		LeaveAndReturnToMainMenu();
	}
}

void UEscapeMenu::OnPlayerLeaveGame()
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

void UEscapeMenu::LeaveButtonClicked()
{
	if (LeaveButton)
	{
		LeaveButton->SetIsEnabled(false);
	}

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* FirstPlayerController = World->GetFirstPlayerController())
		{
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FirstPlayerController->GetPawn()))
			{
				BlasterCharacter->OnLeaveGame.AddDynamic(this, &UEscapeMenu::OnPlayerLeaveGame);
				BlasterCharacter->ServerInitiateLeavingGame();
			}
			else
			{
				LeaveButton->SetIsEnabled(true);
			}
		}
	}
}

void UEscapeMenu::QuitButtonClicked()
{
	if (QuitButton)
	{
		QuitButton->SetIsEnabled(false);
	}

	bLeaveAndQuit = true;

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* FirstPlayerController = World->GetFirstPlayerController())
		{
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FirstPlayerController->GetPawn()))
			{
				BlasterCharacter->OnLeaveGame.AddDynamic(this, &UEscapeMenu::OnPlayerLeaveGame);
				BlasterCharacter->ServerInitiateLeavingGame();
			}
			else
			{
				QuitButton->SetIsEnabled(true);
				bLeaveAndQuit = false;
			}
		}
	}
}

void UEscapeMenu::LeaveAndReturnToMainMenu()
{
	if (const UWorld* World = GetWorld())
	{
		if (AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>())
		{
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr
				                   ? World->GetFirstPlayerController()
				                   : PlayerController.Get();
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UEscapeMenu::LeaveAndQuit()
{
	if (const UWorld* World = GetWorld())
	{
		PlayerController = PlayerController == nullptr
			                   ? World->GetFirstPlayerController()
			                   : PlayerController.Get();
		if (PlayerController)
		{
			UKismetSystemLibrary::QuitGame(World, PlayerController, EQuitPreference::Quit, true);
		}
	}
}
