// Fill out your copyright notice in the Description page of Project Settings.

#include "Menu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UMenu::MenuSetup(const TSoftObjectPtr<UWorld> LobbyLevel, const int32 NumberOfPublicConnections)
{
	LobbyPath = LobbyLevel.GetLongPackageName().Append("?listen");
	NumPublicConnections = NumberOfPublicConnections;

	AddToViewport();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	SetPlayerInputMode();
	SetMultiplayerSessionSubsystem();
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (NumberOfPlayersSlider)
	{
		NumberOfPlayersSlider->OnValueChanged.AddDynamic(this, &UMenu::OnNumberOfPlayersChanged);
	}

	if (DeathmatchButton)
	{
		DeathmatchButton->OnClicked.AddDynamic(this, &UMenu::OnDeathmatchButtonClicked);
	}

	if (TeamDeathmatchButton)
	{
		TeamDeathmatchButton->OnClicked.AddDynamic(this, &UMenu::OnTeamDeathmatchButtonClicked);
	}

	if (CaptureTheFlagButton)
	{
		CaptureTheFlagButton->OnClicked.AddDynamic(this, &UMenu::OnCaptureTheFlagButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenu::OnJoinButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTeardown();

	Super::NativeDestruct();
}

void UMenu::OnCreateSession(const bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		Log(FColor::Green, FString(TEXT("Game session created successfully.")));

		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(LobbyPath);
		}
	}
	else
	{
		Log(FColor::Red, FString(TEXT("Failed to create game session.")));

		DeathmatchButton->SetIsEnabled(true);
		TeamDeathmatchButton->SetIsEnabled(true);
		CaptureTheFlagButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, const bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		Log(FColor::Red, FString(TEXT("Multiplayer sessions subsystem is missing.")));
		JoinButton->SetIsEnabled(true);
		return;
	}

	if (bWasSuccessful)
	{
		for (auto Result : SessionResults)
		{
			Log(FColor::Green, FString(TEXT("Game session found successfully.")));
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}

	Log(FColor::Red, FString(TEXT("Failed to find game session.")));
	JoinButton->SetIsEnabled(true);
}

void UMenu::OnJoinSession(const EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		Log(FColor::Red, FString(TEXT("Failed to join game session.")));

		JoinButton->SetIsEnabled(true);
	}
	else
	{
		Log(FColor::Green, FString(TEXT("Game session joined successfully.")));
	}

	if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PlayerController->ClientTravel(Address, TRAVEL_Absolute);
			}
		}
	}
}

void UMenu::OnStartSession(const bool bWasSuccessful)
{
}

void UMenu::OnDestroySession(const bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(LobbyPath);
		}

		Log(FColor::Green, FString(TEXT("Game session was destroyed successfully.")));
	}
	else
	{
		Log(FColor::Red, FString(TEXT("Failed to destroy game session.")));
	}
}

void UMenu::OnNumberOfPlayersChanged(const float Value)
{
	NumPublicConnections = FMath::RoundToInt(Value);

	if (NumberOfPlayersText)
	{
		NumberOfPlayersText->SetText(FText::FromString(FString::Printf(TEXT("Players: %d"), NumPublicConnections)));
	}
}

void UMenu::OnDeathmatchButtonClicked()
{
	DeathmatchButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, TEXT("Deathmatch"));
	}
}

void UMenu::OnTeamDeathmatchButtonClicked()
{
	TeamDeathmatchButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, TEXT("TeamDeathmatch"));
	}
}

void UMenu::OnCaptureTheFlagButtonClicked()
{
	CaptureTheFlagButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, TEXT("CaptureTheFlag"));
	}
}

void UMenu::OnJoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(INT32_MAX);
	}
}

void UMenu::SetPlayerInputMode()
{
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
}

void UMenu::SetMultiplayerSessionSubsystem()
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &UMenu::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &UMenu::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &UMenu::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &UMenu::OnStartSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UMenu::OnDestroySession);
	}
}

void UMenu::MenuTeardown()
{
	RemoveFromParent();

	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			const FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void UMenu::Log(const FColor DisplayColor, const FString& DebugMessage)
{
	if (GEngine && bEnableLogging)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, DisplayColor, DebugMessage, false);
	}
}
