// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;

UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddToEliminations(int32 EliminationsAmount);
	void AddToCaptures(int32 CapturesAmount);
	void AddToAssists(int32 AssistsAmount);
	void AddToDefeats(int32 DefeatsAmount);

	void SetTeam(ETeam NewTeam);

private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> PlayerController;

	UPROPERTY(Replicated)
	int32 Captures;

	UPROPERTY(Replicated)
	int32 Eliminations;

	UPROPERTY(Replicated)
	float EliminationTimestamp;

	UPROPERTY(Replicated)
	int32 Assists;

	UPROPERTY(Replicated)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();

public:
	int32 GetCaptures() const { return Captures; }
	int32 GetEliminations() const { return Eliminations; }
	float GetEliminationTimestamp() const { return EliminationTimestamp; }
	void SetEliminationTimestamp(const float Timestamp) { EliminationTimestamp = Timestamp; }
	int32 GetAssists() const { return Assists; }
	int32 GetDefeats() const { return Defeats; }
	ETeam GetTeam() const { return Team; }
};
