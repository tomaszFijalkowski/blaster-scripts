// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameUserSettings/BlasterGameUserSettings.h"
#include "Blaster/HUD/EscapeMenu.h"
#include "Blaster/HUD/LobbyHUD.h"
#include "Blaster/HUD/WaitingForPlayersWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void ALobbyPlayerController::PawnLeavingGame()
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->LeaveGame();
	}

	Super::PawnLeavingGame();
}

void ALobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ALobbyPlayerController::OnNumberOfPlayersChanged(const int32 NumPlayers, const int32 MaxPlayers)
{
	SetWaitingForPlayersWidget(NumPlayers, MaxPlayers);
}

void ALobbyPlayerController::ClientOnNumberOfPlayersChanged_Implementation(
	const int32 NumberOfPlayers, const int32 MaxPlayers)
{
	OnNumberOfPlayersChanged(NumberOfPlayers, MaxPlayers);
}

void ALobbyPlayerController::SetPostProcessingEnabled(const bool bEnabled)
{
	TArray<AActor*> PostProcessVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APostProcessVolume::StaticClass(), PostProcessVolumes);

	for (AActor* Actor : PostProcessVolumes)
	{
		if (APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor))
		{
			// Skip volumes with post-process materials (outlines)
			if (Volume->Settings.WeightedBlendables.Array.Num() > 0)
			{
				continue;
			}

			Volume->bEnabled = bEnabled;
		}
	}
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	LobbyHUD = Cast<ALobbyHUD>(GetHUD());

	InitializeEnhancedInput();

	if (const UBlasterGameUserSettings* GameUserSettings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		SetPostProcessingEnabled(GameUserSettings->IsPostProcessingEnabled());
	}
}

void ALobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(EscapeAction, ETriggerEvent::Triggered, this,
		                                   &ALobbyPlayerController::Escape);
	}
}

void ALobbyPlayerController::Escape(const FInputActionValue& Value)
{
	if (EscapeMenuClass == nullptr)
	{
		return;
	}

	if (EscapeMenu == nullptr)
	{
		EscapeMenu = CreateWidget<UEscapeMenu>(this, EscapeMenuClass);
	}

	if (EscapeMenu)
	{
		if (!EscapeMenu->IsSettingsPanelOpen())
		{
			bIsEscapeMenuOpen = !bIsEscapeMenuOpen;

			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
			{
				if (bIsEscapeMenuOpen)
				{
					EscapeMenu->Setup();
					BlasterCharacter->DisableInput(this);
				}
				else
				{
					EscapeMenu->Teardown();
					BlasterCharacter->EnableInput(this);
				}
			}
		}
		else
		{
			EscapeMenu->CloseSettingsPanel();
		}
	}
}

void ALobbyPlayerController::InitializeEnhancedInput() const
{
	if (GEngine == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	if (const ULocalPlayer* FirstGamePlayer = GEngine->GetFirstGamePlayer(GetWorld()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(FirstGamePlayer))
		{
			Subsystem->AddMappingContext(UIMappingContext, 0);
		}
	}
}

void ALobbyPlayerController::SetWaitingForPlayersWidget(const int32 NumberOfPlayers, const int32 MaxPlayers)
{
	if (!IsLocalController())
	{
		return;
	}

	LobbyHUD = LobbyHUD == nullptr ? Cast<ALobbyHUD>(GetHUD()) : LobbyHUD.Get();
	if (LobbyHUD)
	{
		if (LobbyHUD->GetWaitingForPlayersWidget() == nullptr)
		{
			LobbyHUD->AddWaitingForPlayersWidget();
		}

		const TObjectPtr<UWaitingForPlayersWidget> WaitingForPlayersWidget = LobbyHUD->GetWaitingForPlayersWidget();
		if (WaitingForPlayersWidget && WaitingForPlayersWidget->WaitingForPlayersBox &&
			WaitingForPlayersWidget->WaitingForPlayersNumber)
		{
			const FString NumberOfPlayersText = FString::Printf(TEXT("%d/%d"), NumberOfPlayers, MaxPlayers);
			WaitingForPlayersWidget->WaitingForPlayersNumber->SetText(FText::FromString(NumberOfPlayersText));
		}

		if (NumberOfPlayers == MaxPlayers)
		{
			if (WaitingForPlayersWidget && WaitingForPlayersWidget->MatchStartingBox)
			{
				WaitingForPlayersWidget->MatchStartingBox->SetVisibility(ESlateVisibility::Visible);
				WaitingForPlayersWidget->PlayMatchStartingAnimation();
			}
		}
	}
}
