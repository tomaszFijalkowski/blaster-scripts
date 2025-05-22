// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class UInputMappingContext;
class UEscapeMenu;
class UInputAction;
struct FInputActionValue;
class ALobbyHUD;
class UWaitingForPlayersWidget;

UCLASS()
class BLASTER_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PawnLeavingGame() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetPostProcessingEnabled(bool bEnabled);
	void OnNumberOfPlayersChanged(int32 NumPlayers, int32 MaxPlayers);

	UFUNCTION(Client, Reliable)
	void ClientOnNumberOfPlayersChanged(int32 NumberOfPlayers, int32 MaxPlayers);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void Escape(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UIMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> EscapeAction;

private:
	void InitializeEnhancedInput() const;
	void SetWaitingForPlayersWidget(int32 NumberOfPlayers, int32 MaxPlayers);

	UPROPERTY()
	TObjectPtr<ALobbyHUD> LobbyHUD;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> EscapeMenuClass;

	UPROPERTY()
	TObjectPtr<UEscapeMenu> EscapeMenu;

	bool bIsEscapeMenuOpen = false;
};
