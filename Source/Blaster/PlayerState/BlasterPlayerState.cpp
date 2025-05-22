// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Captures);
	DOREPLIFETIME(ABlasterPlayerState, Eliminations);
	DOREPLIFETIME(ABlasterPlayerState, EliminationTimestamp);
	DOREPLIFETIME(ABlasterPlayerState, Assists);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::AddToCaptures(const int32 CapturesAmount)
{
	Captures += CapturesAmount;
}

void ABlasterPlayerState::AddToEliminations(const int32 EliminationsAmount)
{
	Eliminations += EliminationsAmount;
	SetEliminationTimestamp(GetWorld()->GetTimeSeconds());
}

void ABlasterPlayerState::AddToAssists(const int32 AssistsAmount)
{
	Assists += AssistsAmount;
}

void ABlasterPlayerState::AddToDefeats(const int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
}

void ABlasterPlayerState::SetTeam(const ETeam NewTeam)
{
	Team = NewTeam;

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(Team);
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetTeamColor(Team);
	}
}
