// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "AnnouncementWidget.h"
#include "CharacterOverlay.h"
#include "NotificationWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "GameFramework/PlayerState.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (GEngine == nullptr)
	{
		return;
	}

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	const float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
	DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, FVector2D(0.f, 0.f), HUDPackage.CrosshairColor);
	DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, FVector2D(-SpreadScaled, 0.f), HUDPackage.CrosshairColor);
	DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, FVector2D(SpreadScaled, 0.f), HUDPackage.CrosshairColor);
	DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, FVector2D(0.f, -SpreadScaled), HUDPackage.CrosshairColor);
	DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, FVector2D(0.f, SpreadScaled), HUDPackage.CrosshairColor);
}

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	PlayerController = PlayerController == nullptr ? GetOwningPlayerController() : PlayerController.Get();
	if (PlayerController && CharacterOverlayClass && CharacterOverlay == nullptr)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		if (CharacterOverlay)
		{
			CharacterOverlay->AddToViewport();
		}
	}
}

void ABlasterHUD::AddAnnouncement()
{
	PlayerController = PlayerController == nullptr ? GetOwningPlayerController() : PlayerController.Get();
	if (PlayerController && AnnouncementClass && Announcement == nullptr)
	{
		Announcement = CreateWidget<UAnnouncementWidget>(PlayerController, AnnouncementClass);
		if (Announcement)
		{
			Announcement->AddToViewport();
		}
	}
}

void ABlasterHUD::AddEliminationNotification(const APlayerState* EliminatingPlayer, const APlayerState* AssistingPlayer,
                                             const APlayerState* Victim, const bool bIsImportant,
                                             const bool bSelfElimination)
{
	check(Victim);

	// Prevent multiple notifications from being added at the same time (overlapping each other)
	const float CurrentNotificationTimestamp = GetWorld()->GetTimeSeconds();
	if (CurrentNotificationTimestamp == PreviousNotificationTimestamp)
	{
		FTimerHandle RetryTimer;
		FTimerDelegate RetryDelegate;

		const FName FunctionName = GET_FUNCTION_NAME_CHECKED(ABlasterHUD, AddEliminationNotification);
		RetryDelegate.BindUFunction(
			this,
			FunctionName,
			EliminatingPlayer,
			AssistingPlayer,
			Victim,
			bIsImportant,
			bSelfElimination
		);

		GetWorldTimerManager().SetTimer(RetryTimer, RetryDelegate, NotificationInterval, false);
		return;
	}

	PlayerController = PlayerController == nullptr ? GetOwningPlayerController() : PlayerController.Get();
	if (PlayerController && NotificationClass)
	{
		if (TObjectPtr<UNotificationWidget> NotificationToAdd =
			CreateWidget<UNotificationWidget>(PlayerController, NotificationClass))
		{
			NotificationToAdd->SetEliminationText(
				EliminatingPlayer, AssistingPlayer, Victim, bIsImportant, bSelfElimination);
			NotificationToAdd->AddToViewport();
			NotificationToAdd->PlayNotificationAnimation();

			for (const auto Notification : Notifications)
			{
				if (Notification && Notification->NotificationBox)
				{
					if (UCanvasPanelSlot* CanvasSlot =
						UWidgetLayoutLibrary::SlotAsCanvasSlot(Notification->NotificationBox))
					{
						const FVector2D Position = CanvasSlot->GetPosition();
						const FVector2D Size = Notification->NotificationBox->GetDesiredSize();
						const FVector2D NewPosition(
							CanvasSlot->GetPosition().X,
							Position.Y + Size.Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			Notifications.Add(NotificationToAdd);

			FTimerHandle NotificationTimer;
			FTimerDelegate NotificationDelegate;

			const FName FunctionName = GET_FUNCTION_NAME_CHECKED(ABlasterHUD, NotificationTimerFinished);
			NotificationDelegate.BindUFunction(this, FunctionName, NotificationToAdd);

			GetWorldTimerManager().SetTimer(NotificationTimer, NotificationDelegate, NotificationDuration, false);
		}
	}

	PreviousNotificationTimestamp = CurrentNotificationTimestamp;
}

void ABlasterHUD::AddFlagCaptureNotification(const APlayerState* ScoringPlayer)
{
	// Prevent multiple notifications from being added at the same time (overlapping each other)
	const float CurrentNotificationTimestamp = GetWorld()->GetTimeSeconds();
	if (CurrentNotificationTimestamp == PreviousNotificationTimestamp)
	{
		FTimerHandle RetryTimer;
		FTimerDelegate RetryDelegate;

		const FName FunctionName = GET_FUNCTION_NAME_CHECKED(ABlasterHUD, AddFlagCaptureNotification);
		RetryDelegate.BindUFunction(
			this,
			FunctionName,
			ScoringPlayer
		);

		GetWorldTimerManager().SetTimer(RetryTimer, RetryDelegate, NotificationInterval, false);
		return;
	}

	PlayerController = PlayerController == nullptr ? GetOwningPlayerController() : PlayerController.Get();
	if (PlayerController && NotificationClass)
	{
		if (TObjectPtr<UNotificationWidget> NotificationToAdd =
			CreateWidget<UNotificationWidget>(PlayerController, NotificationClass))
		{
			NotificationToAdd->SetFlagCaptureText(ScoringPlayer);
			NotificationToAdd->AddToViewport();
			NotificationToAdd->PlayNotificationAnimation();

			for (const auto Notification : Notifications)
			{
				if (Notification && Notification->NotificationBox)
				{
					if (UCanvasPanelSlot* CanvasSlot =
						UWidgetLayoutLibrary::SlotAsCanvasSlot(Notification->NotificationBox))
					{
						const FVector2D Position = CanvasSlot->GetPosition();
						const FVector2D Size = Notification->NotificationBox->GetDesiredSize();
						const FVector2D NewPosition(
							CanvasSlot->GetPosition().X,
							Position.Y + Size.Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			Notifications.Add(NotificationToAdd);

			FTimerHandle NotificationTimer;
			FTimerDelegate NotificationDelegate;

			const FName FunctionName = GET_FUNCTION_NAME_CHECKED(ABlasterHUD, NotificationTimerFinished);
			NotificationDelegate.BindUFunction(this, FunctionName, NotificationToAdd);

			GetWorldTimerManager().SetTimer(NotificationTimer, NotificationDelegate, NotificationDuration, false);
		}
	}

	PreviousNotificationTimestamp = CurrentNotificationTimestamp;
}

void ABlasterHUD::NotificationTimerFinished(UNotificationWidget* NotificationToRemove)
{
	if (NotificationToRemove)
	{
		NotificationToRemove->RemoveFromParent();
		Notifications.Remove(NotificationToRemove);
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D ViewportCenter, const FVector2D Spread,
                                const FLinearColor Color)
{
	if (Texture)
	{
		const float TextureWidth = Texture->GetSizeX();
		const float TextureHeight = Texture->GetSizeY();
		const FVector2D TextureDrawPoint(
			ViewportCenter.X - TextureWidth / 2.f + Spread.X,
			ViewportCenter.Y - TextureHeight / 2.f + Spread.Y
		);

		DrawTexture(
			Texture,
			TextureDrawPoint.X,
			TextureDrawPoint.Y,
			TextureWidth,
			TextureHeight,
			0.f,
			0.f,
			1.f,
			1.f,
			Color
		);
	}
}
