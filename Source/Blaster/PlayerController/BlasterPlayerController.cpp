// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Constants/Constants.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/GameUserSettings/BlasterGameUserSettings.h"
#include "Blaster/HUD/AnnouncementWidget.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/EscapeMenu.h"
#include "Blaster/HUD/ScoreboardWidget.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

struct FBuffDurationEntry;
class ABlasterGameState;

void ABlasterPlayerController::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckPing(DeltaTime);
	CheckTimeSync(DeltaTime);
	SetHUDTime();

	PollInitCharacterOverlay();
	PollInitTeamScore();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bIsTeamsMatch);
}

void ABlasterPlayerController::PawnLeavingGame()
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->LeaveGame();
	}

	Super::PawnLeavingGame();
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

float ABlasterPlayerController::GetServerTime()
{
	return GetWorld()->GetTimeSeconds() + (HasAuthority() ? 0.f : ClientServerDelta);
}

void ABlasterPlayerController::SetPostProcessingEnabled(const bool bEnabled)
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

void ABlasterPlayerController::SetHUDHealth(const float Health, const float MaxHealth, const float HealthBarPercent)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->HealthBar && CharacterOverlay->HealthText)
	{
		CharacterOverlay->HealthBar->SetPercent(HealthBarPercent);

		const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health),
		                                           FMath::CeilToInt(MaxHealth));
		CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(const float Shield, const float MaxShield)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->ShieldBar)
	{
		const float ShieldPercent = Shield / MaxShield;
		CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		const FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield),
		                                           FMath::CeilToInt(MaxShield));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDRespawnBoxVisibility(const bool bVisibility)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->RespawnBox)
	{
		const ESlateVisibility Visibility = bVisibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		CharacterOverlay->RespawnBox->SetVisibility(Visibility);
	}
}

void ABlasterPlayerController::SetHUDRespawnTime(const float RespawnTime)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->RespawnTime)
	{
		const FString RespawnTimeText = FString::Printf(TEXT("%d"), FMath::CeilToInt(RespawnTime));
		CharacterOverlay->RespawnTime->SetText(FText::FromString(RespawnTimeText));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(const int32 Ammo, const FLinearColor Color)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->WeaponAmmoAmount)
	{
		const FString WeaponAmmoText = FString::Printf(TEXT("%d"), Ammo);
		CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));

		if (Color != FLinearColor::Black)
		{
			CharacterOverlay->WeaponAmmoAmount->SetColorAndOpacity(FSlateColor(Color));

			if (CharacterOverlay->CarriedAmmoAmount)
			{
				CharacterOverlay->CarriedAmmoAmount->SetColorAndOpacity(FSlateColor(Color));
			}
		}
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(const int32 Ammo)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->CarriedAmmoAmount)
	{
		const FString CarriedAmmoText = FString::Printf(TEXT(" %d"), Ammo);
		CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(const float CountdownTime)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->MatchCountdownText)
	{
		const int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		const int32 Seconds = CountdownTime - Minutes * 60;
		const FString CountdownText = FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
		CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));

		if (CountdownTime <= COUNTDOWN_BLINKING_THRESHOLD && CountdownTime > 1.f)
		{
			CharacterOverlay->PlayMatchCountdownAnimation();
		}
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(const float CountdownTime)
{
	const TObjectPtr<UAnnouncementWidget> Announcement = GetValidAnnouncement();
	if (Announcement && Announcement->AnnouncementValueText)
	{
		const int32 Seconds = FMath::FloorToInt(CountdownTime);
		const FString CountdownText = FString::Printf(TEXT("%d"), Seconds);
		Announcement->AnnouncementValueText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(const int32 Grenades)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->GrenadesText)
	{
		const FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::SetHUDTeamScoreVisibility() const
{
	check(CharacterOverlay)
	check(BlasterPlayerState)

	if (CharacterOverlay->BlueTeamScoreBox && BlasterPlayerState->GetTeam() == ETeam::ET_BlueTeam)
	{
		CharacterOverlay->BlueTeamScoreBox->SetVisibility(ESlateVisibility::Visible);
	}

	if (CharacterOverlay->RedTeamScoreBox && BlasterPlayerState->GetTeam() == ETeam::ET_RedTeam)
	{
		CharacterOverlay->RedTeamScoreBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(const int32 Score)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->BlueTeamScoreLeft && CharacterOverlay->RedTeamScoreRight)
	{
		const FString BlueTeamScoreLeftText = FString::Printf(TEXT("%d"), Score);
		CharacterOverlay->BlueTeamScoreLeft->SetText(FText::FromString(BlueTeamScoreLeftText));

		const FString RedTeamScoreRightText = FString::Printf(TEXT("%d"), Score);
		CharacterOverlay->RedTeamScoreRight->SetText(FText::FromString(RedTeamScoreRightText));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(const int32 Score)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->RedTeamScoreLeft && CharacterOverlay->BlueTeamScoreRight)
	{
		const FString RedTeamScoreLeftText = FString::Printf(TEXT("%d"), Score);
		CharacterOverlay->RedTeamScoreLeft->SetText(FText::FromString(RedTeamScoreLeftText));

		const FString BlueTeamScoreRightText = FString::Printf(TEXT("%d"), Score);
		CharacterOverlay->BlueTeamScoreRight->SetText(FText::FromString(BlueTeamScoreRightText));
	}
}

void ABlasterPlayerController::SetHUDReloadProgress(const float ReloadProgress, const float ReloadDuration)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->ReloadProgressBar)
	{
		const float ReloadProgressPercent = ReloadProgress / ReloadDuration;
		CharacterOverlay->ReloadProgressBar->SetPercent(ReloadProgressPercent);
	}
}

void ABlasterPlayerController::SetHUDReloadProgressVisibility(const bool bVisibility)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->ReloadProgressBar)
	{
		const ESlateVisibility Visibility = bVisibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		CharacterOverlay->ReloadProgressBar->SetVisibility(Visibility);
	}
}

void ABlasterPlayerController::SetHUDBuffDurationBars(TArray<FBuffDurationEntry> BuffDurationEntries)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay)
	{
		const int32 BuffDurationEntriesNum = FMath::Min(BuffDurationEntries.Num(), MAX_BUFF_DURATION_ENTRIES);
		for (int32 Index = 0; Index < BuffDurationEntriesNum; ++Index)
		{
			const FBuffDurationEntry BuffDurationEntry = BuffDurationEntries[Index];

			if (BuffDurationEntries[Index].RemainingTime > MIN_BUFF_REMAINING_TIME / 2.f)
			{
				if (Index == 0)
				{
					UpdateBuffDurationBar(CharacterOverlay->BuffDurationBar1, BuffDurationEntry);
				}
				else if (Index == 1)
				{
					UpdateBuffDurationBar(CharacterOverlay->BuffDurationBar2, BuffDurationEntry);
				}
				else if (Index == 2)
				{
					UpdateBuffDurationBar(CharacterOverlay->BuffDurationBar3, BuffDurationEntry);
				}
				else if (Index == 3)
				{
					UpdateBuffDurationBar(CharacterOverlay->BuffDurationBar4, BuffDurationEntry);
				}
			}
		}

		CollapseRemainingBuffDurationBars(BuffDurationEntriesNum);
	}
}

void ABlasterPlayerController::OnMatchStateSet(const FName State, const bool bInIsTeamsMatch)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bInIsTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
}

void ABlasterPlayerController::BroadcastElimination(APlayerState* EliminatingPlayer, APlayerState* AssistingPlayer,
                                                    APlayerState* Victim, const bool bSelfElimination)
{
	ClientElimination(EliminatingPlayer, AssistingPlayer, Victim, bSelfElimination);
}

void ABlasterPlayerController::BroadcastFlagCapture(ABlasterPlayerState* ScoringPlayer)
{
	ClientFlagCapture(ScoringPlayer);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	PlayerState = PlayerState == nullptr ? Cast<APlayerState>(PlayerState) : PlayerState.Get();
	if (PlayerState)
	{
		const float PingAmount = PlayerState->GetPingInMilliseconds();
		SetHUDPingAmount(PingAmount);

		const bool bIsHighPing = PingAmount > HighPingThreshold;
		EnableHUDHighImage(bIsHighPing);
	}

	if (const UBlasterGameUserSettings* GameUserSettings = UBlasterGameUserSettings::GetGameUserSettings())
	{
		SetPostProcessingEnabled(GameUserSettings->IsPostProcessingEnabled());
	}

	ServerCheckMatchState();
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(EscapeAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterPlayerController::Escape);

		EnhancedInputComponent->BindAction(TabAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterPlayerController::Tab);
	}
}

void ABlasterPlayerController::Escape(const FInputActionValue& Value)
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
					BlasterCharacter->StopAiming();
					BlasterCharacter->StopFiring();

					if (ScoreboardWidget)
					{
						ScoreboardWidget->Teardown();
					}
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

void ABlasterPlayerController::Tab(const FInputActionValue& Value)
{
	if (ScoreboardWidgetClass == nullptr || bIsEscapeMenuOpen)
	{
		return;
	}

	if (ScoreboardWidget == nullptr)
	{
		ScoreboardWidget = CreateWidget<UScoreboardWidget>(this, ScoreboardWidgetClass);
	}

	const bool bTabPressed = Value.Get<bool>();

	if (ScoreboardWidget)
	{
		if (bTabPressed)
		{
			ScoreboardWidget->Setup();
			ScoreboardWidget->UpdateScoreboard();

			StartScoreboardUpdates();
		}
		else
		{
			ScoreboardWidget->Teardown();

			StopScoreboardUpdates();
		}
	}
}

void ABlasterPlayerController::PollInitCharacterOverlay()
{
	if (bIsCharacterOverlayInitialized)
	{
		return;
	}

	if (CharacterOverlay == nullptr)
	{
		CharacterOverlay = GetValidCharacterOverlay();
	}

	if (CharacterOverlay)
	{
		if (bInitializeHealth)
		{
			SetHUDHealth(HUDHealth, HUDMaxHealth, HUDHealth / HUDMaxHealth);
		}

		if (bInitializeShield)
		{
			SetHUDShield(HUDShield, HUDMaxShield);
		}

		if (bInitializeWeaponAmmo)
		{
			SetHUDWeaponAmmo(HUDWeaponAmmo);
		}

		if (bInitializeCarriedAmmo)
		{
			SetHUDCarriedAmmo(HUDCarriedAmmo);
		}

		const ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
		if (BlasterCharacter && BlasterCharacter->GetCombatComponent() && bInitializeGrenades)
		{
			const int32 ThrowingGrenades = BlasterCharacter->GetCombatComponent()->GetThrowingGrenades();
			SetHUDGrenades(ThrowingGrenades);
		}

		bIsCharacterOverlayInitialized = true;
	}
}

void ABlasterPlayerController::PollInitTeamScore()
{
	if (bIsTeamScoreInitialized)
	{
		return;
	}

	if (CharacterOverlay == nullptr)
	{
		CharacterOverlay = GetValidCharacterOverlay();
	}

	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
	}

	if (CharacterOverlay && BlasterPlayerState && bIsTeamsMatch)
	{
		if (BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
		{
			SetHUDTeamScoreVisibility();

			bIsTeamScoreInitialized = true;
		}
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	if (HasAuthority())
	{
		if (const ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			LevelStartingTime = BlasterGameMode->GetLevelStartingTime();
		}
	}

	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = LevelStartingTime + WarmupTime - GetServerTime();
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = LevelStartingTime + WarmupTime + MatchTime - GetServerTime();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = LevelStartingTime + WarmupTime + MatchTime + CooldownTime - GetServerTime();
	}

	const uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(FMath::Max(0.f, TimeLeft + 1.f));
		}

		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(FMath::Max(0.f, TimeLeft + 1.f));
		}
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::HandleMatchHasStarted(const bool bInIsTeamsMatch)
{
	InitializeEnhancedInput();

	if (HasAuthority())
	{
		bIsTeamsMatch = bInIsTeamsMatch;
	}

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD)
	{
		if (BlasterHUD->GetCharacterOverlay() == nullptr)
		{
			BlasterHUD->AddCharacterOverlay();
		}

		if (const TObjectPtr<UAnnouncementWidget> Announcement = BlasterHUD->GetAnnouncement())
		{
			Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleMatchCooldown()
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay)
	{
		CharacterOverlay->RemoveFromParent();
		const TObjectPtr<UAnnouncementWidget> Announcement = GetValidAnnouncement();
		if (Announcement && Announcement->AnnouncementText && Announcement->InfoBox)
		{
			Announcement->SetVisibility(ESlateVisibility::Visible);
			Announcement->InfoBox->SetVisibility(ESlateVisibility::Hidden);
			Announcement->AnnouncementText->SetText(FText::FromName(Announcement::NewMatchStart));

			if (bIsTeamsMatch)
			{
				ShowWinningTeam(Announcement);
			}
			else
			{
				ShowTopPlayers(Announcement);
			}
		}
	}

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->HandleCooldown();
	}
}

void ABlasterPlayerController::ClientFlagCapture_Implementation(ABlasterPlayerState* ScoringPlayer)
{
	if (ScoringPlayer)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
		if (BlasterHUD)
		{
			BlasterHUD->AddFlagCaptureNotification(ScoringPlayer);
		}
	}
}

void ABlasterPlayerController::ClientElimination_Implementation(APlayerState* EliminatingPlayer,
                                                                APlayerState* AssistingPlayer, APlayerState* Victim,
                                                                const bool bSelfElimination)
{
	const APlayerState* Self = GetPlayerState<APlayerState>();
	if (Victim && Self)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
		if (BlasterHUD)
		{
			const bool bIsImportant = EliminatingPlayer == Self || AssistingPlayer == Self || Victim == Self;
			BlasterHUD->AddEliminationNotification(
				EliminatingPlayer,
				AssistingPlayer,
				Victim,
				bIsImportant,
				bSelfElimination
			);
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(const float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(const float TimeOfClientRequest,
                                                                     const float TimeOfServerReceivedClientRequest)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = RoundTripTime / 2.f;

	const float CurrentServerTime = TimeOfServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if (const ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		MatchState = GameMode->GetMatchState();
		WarmupTime = GameMode->GetWarmupTime();
		MatchTime = GameMode->GetMatchTime();
		CooldownTime = GameMode->GetCooldownTime();
		LevelStartingTime = GameMode->GetLevelStartingTime();
		bIsTeamsMatch = GameMode->IsTeamsMatch();

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime, bIsTeamsMatch);
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(const FName InMatchState, const float InWarmupTime,
                                                                const float InMatchTime, const float InCooldownTime,
                                                                const float InLevelStartingTime,
                                                                const bool bInIsTeamsMatch)
{
	MatchState = InMatchState;
	WarmupTime = InWarmupTime;
	MatchTime = InMatchTime;
	CooldownTime = InCooldownTime;
	LevelStartingTime = InLevelStartingTime;
	bIsTeamsMatch = bInIsTeamsMatch;

	OnMatchStateSet(MatchState, bIsTeamsMatch);

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD && BlasterHUD->GetAnnouncement() == nullptr && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::SetHUDPingAmount(const float PingAmount)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->PingAmount)
	{
		const int32 RoundedPingAmount = FMath::RoundToInt(PingAmount);
		if (RoundedPingAmount >= HighPingThreshold)
		{
			const FString PingAmountText = FString::Printf(TEXT("%d"), RoundedPingAmount);
			CharacterOverlay->PingAmount->SetText(FText::FromString(PingAmountText));

			if (bIsPingDisplayed == false)
			{
				CharacterOverlay->PlayPingAmountFadeInAnimation();
			}
			bIsPingDisplayed = true;
		}
		else
		{
			CharacterOverlay->PingAmount->SetText(FText::GetEmpty());
			bIsPingDisplayed = false;
		}
	}
}

void ABlasterPlayerController::EnableHUDHighImage(const bool bEnable)
{
	CharacterOverlay = GetValidCharacterOverlay();
	if (CharacterOverlay && CharacterOverlay->HighPingImage)
	{
		if (bEnable)
		{
			CharacterOverlay->HighPingImage->SetOpacity(1.f);
			CharacterOverlay->PlayHighPingAnimation();
		}
		else
		{
			CharacterOverlay->HighPingImage->SetOpacity(0.f);
			CharacterOverlay->StopHighPingAnimation();
		}
	}
}

void ABlasterPlayerController::InitializeEnhancedInput() const
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

void ABlasterPlayerController::CheckPing(const float DeltaTime)
{
	HighPingRunningTime += DeltaTime;

	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? Cast<APlayerState>(PlayerState) : PlayerState.Get();
		if (PlayerState)
		{
			const float PingAmount = PlayerState->GetPingInMilliseconds();
			SetHUDPingAmount(PingAmount);

			const bool bIsHighPing = PingAmount > HighPingThreshold;
			EnableHUDHighImage(bIsHighPing);
		}
		HighPingRunningTime = 0.f;
	}
}

void ABlasterPlayerController::CheckTimeSync(const float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ShowTopPlayers(const TObjectPtr<UAnnouncementWidget>& Announcement)
{
	const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(
		UGameplayStatics::GetGameState(GetWorld()));

	const bool bValidTopPlayersBox = Announcement->TopPlayersBox && Announcement->FirstPlaceBox &&
		Announcement->FirstPlaceScore && Announcement->FirstPlaceText &&
		Announcement->SecondPlaceBox && Announcement->SecondPlaceScore &&
		Announcement->SecondPlaceText && Announcement->ThirdPlaceBox &&
		Announcement->ThirdPlaceScore && Announcement->ThirdPlaceText;

	if (BlasterGameState && bValidTopPlayersBox)
	{
		const TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->GetTopPlayers();
		if (TopPlayers.Num() > 0)
		{
			Announcement->TopPlayersBox->SetVisibility(ESlateVisibility::Visible);
			Announcement->FirstPlaceBox->SetVisibility(ESlateVisibility::Visible);

			const FString FirstPlaceScore = FString::Printf(TEXT("%d"), TopPlayers[0]->GetEliminations());
			Announcement->FirstPlaceScore->SetText(FText::FromString(FirstPlaceScore));

			const FString FirstPlaceText = FString::Printf(TEXT(" %s"), *TopPlayers[0]->GetPlayerName());
			Announcement->FirstPlaceText->SetText(FText::FromString(FirstPlaceText));

			if (TopPlayers.Num() > 1)
			{
				Announcement->SecondPlaceBox->SetVisibility(ESlateVisibility::Visible);

				const FString SecondPlaceScore = FString::Printf(TEXT("%d"), TopPlayers[1]->GetEliminations());
				Announcement->SecondPlaceScore->SetText(FText::FromString(SecondPlaceScore));

				const FString SecondPlaceText = FString::Printf(TEXT(" %s"), *TopPlayers[1]->GetPlayerName());
				Announcement->SecondPlaceText->SetText(FText::FromString(SecondPlaceText));
			}

			if (TopPlayers.Num() > 2)
			{
				Announcement->ThirdPlaceBox->SetVisibility(ESlateVisibility::Visible);

				const FString ThirdPlaceScore =
					FString::Printf(TEXT("%d"), TopPlayers[2]->GetEliminations());
				Announcement->ThirdPlaceScore->SetText(FText::FromString(ThirdPlaceScore));

				const FString ThirdPlaceText = FString::Printf(TEXT(" %s"), *TopPlayers[2]->GetPlayerName());
				Announcement->ThirdPlaceText->SetText(FText::FromString(ThirdPlaceText));
			}
		}
	}
}

void ABlasterPlayerController::ShowWinningTeam(const TObjectPtr<UAnnouncementWidget>& Announcement)
{
	const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(
		UGameplayStatics::GetGameState(GetWorld()));

	BlasterPlayerState = BlasterPlayerState == nullptr
		                     ? GetPlayerState<ABlasterPlayerState>()
		                     : BlasterPlayerState.Get();

	if (BlasterGameState && BlasterPlayerState && Announcement && Announcement->WinningTeamBox)
	{
		Announcement->WinningTeamBox->SetVisibility(ESlateVisibility::Visible);

		const int32 BlueTeamScore = BlasterGameState->GetBlueTeamScore();
		const int32 RedTeamScore = BlasterGameState->GetRedTeamScore();

		const FString BlueTeamScoreText = FString::Printf(TEXT("%d"), BlueTeamScore);
		const FString RedTeamScoreText = FString::Printf(TEXT("%d"), RedTeamScore);

		if (BlueTeamScore > RedTeamScore)
		{
			Announcement->BlueTeamWinsBox->SetVisibility(ESlateVisibility::Visible);
		}
		else if (RedTeamScore > BlueTeamScore)
		{
			Announcement->RedTeamWinsBox->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Announcement->DrawBox->SetVisibility(ESlateVisibility::Visible);
		}

		if (BlasterPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			Announcement->BlueTeamScoreBox->SetVisibility(ESlateVisibility::Visible);
			Announcement->BlueTeamScoreLeft->SetText(FText::FromString(BlueTeamScoreText));
			Announcement->BlueTeamScoreRight->SetText(FText::FromString(RedTeamScoreText));
		}
		else if (BlasterPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			Announcement->RedTeamScoreBox->SetVisibility(ESlateVisibility::Visible);
			Announcement->RedTeamScoreLeft->SetText(FText::FromString(RedTeamScoreText));
			Announcement->RedTeamScoreRight->SetText(FText::FromString(BlueTeamScoreText));
		}
	}
}

TObjectPtr<UCharacterOverlay> ABlasterPlayerController::GetValidCharacterOverlay()
{
	if (CharacterOverlay)
	{
		return CharacterOverlay;
	}

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD)
	{
		return BlasterHUD->GetCharacterOverlay();
	}

	return nullptr;
}

TObjectPtr<UAnnouncementWidget> ABlasterPlayerController::GetValidAnnouncement()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD)
	{
		return BlasterHUD->GetAnnouncement();
	}

	return nullptr;
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
}

void ABlasterPlayerController::UpdateBuffDurationBar(UProgressBar* BuffDurationBar,
                                                     const FBuffDurationEntry& BuffDurationEntry)
{
	if (BuffDurationBar)
	{
		const float BuffDurationPercent = BuffDurationEntry.RemainingTime / BuffDurationEntry.Duration;
		BuffDurationBar->SetPercent(BuffDurationPercent);

		if (BuffDurationEntry.RemainingTime <= 0.f && BuffDurationBar->GetVisibility() != ESlateVisibility::Collapsed)
		{
			BuffDurationBar->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			if (BuffDurationBar->GetFillColorAndOpacity() != BuffDurationEntry.FillColor)
			{
				BuffDurationBar->SetFillColorAndOpacity(BuffDurationEntry.FillColor);
			}

			if (BuffDurationBar->GetVisibility() != ESlateVisibility::Visible)
			{
				BuffDurationBar->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void ABlasterPlayerController::CollapseRemainingBuffDurationBars(const int32 BuffDurationEntriesNum)
{
	check(CharacterOverlay);

	for (int32 Index = BuffDurationEntriesNum; Index < MAX_BUFF_DURATION_ENTRIES; ++Index)
	{
		if (Index == 0 && CharacterOverlay->BuffDurationBar1 &&
			CharacterOverlay->BuffDurationBar1->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CharacterOverlay->BuffDurationBar1->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (Index == 1 && CharacterOverlay->BuffDurationBar2 &&
			CharacterOverlay->BuffDurationBar2->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CharacterOverlay->BuffDurationBar2->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (Index == 2 && CharacterOverlay->BuffDurationBar3 &&
			CharacterOverlay->BuffDurationBar3->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CharacterOverlay->BuffDurationBar3->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (Index == 3 && CharacterOverlay->BuffDurationBar4 &&
			CharacterOverlay->BuffDurationBar4->GetVisibility() != ESlateVisibility::Collapsed)
		{
			CharacterOverlay->BuffDurationBar4->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ABlasterPlayerController::StartScoreboardUpdates()
{
	GetWorldTimerManager().SetTimer(
		ScoreboardUpdateTimer,
		this,
		&ABlasterPlayerController::UpdateScoreboard,
		ScoreboardUpdateInterval,
		true
	);
}

void ABlasterPlayerController::StopScoreboardUpdates()
{
	GetWorldTimerManager().ClearTimer(ScoreboardUpdateTimer);
}

void ABlasterPlayerController::UpdateScoreboard()
{
	if (ScoreboardWidget)
	{
		ScoreboardWidget->UpdateScoreboard();
	}
}
