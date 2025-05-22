// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UNotificationWidget;
class UAnnouncementWidget;
class UCharacterOverlay;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairLeft;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairBottom;

	float CrosshairSpread;
	FLinearColor CrosshairColor;
};

UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	void AddAnnouncement();

	UFUNCTION()
	void AddEliminationNotification(const APlayerState* EliminatingPlayer,
	                                const APlayerState* AssistingPlayer, const APlayerState* Victim, bool bIsImportant,
	                                bool bSelfElimination);

	UFUNCTION()
	void AddFlagCaptureNotification(const APlayerState* ScoringPlayer);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void NotificationTimerFinished(UNotificationWidget* NotificationToRemove);

	void DrawCrosshair(UTexture2D* Texture, const FVector2D ViewportCenter, const FVector2D Spread,
	                   const FLinearColor Color);

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UCharacterOverlay> CharacterOverlayClass;

	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY()
	TObjectPtr<UAnnouncementWidget> Announcement;

	UPROPERTY(EditAnywhere, Category = "Notification")
	TSubclassOf<UNotificationWidget> NotificationClass;

	UPROPERTY(EditAnywhere, Category = "Notification")
	float NotificationDuration = 6.f;

	UPROPERTY(EditAnywhere, Category = "Notification")
	float NotificationInterval = 0.25f; // For simultaneous notifications

	UPROPERTY()
	TArray<UNotificationWidget*> Notifications;

	float PreviousNotificationTimestamp = 0.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE TObjectPtr<UCharacterOverlay> GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE TObjectPtr<UAnnouncementWidget> GetAnnouncement() const { return Announcement; }
};
