// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterSpectatorPawn.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterSpectatorPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetInitialRotation();
}

void ABlasterSpectatorPawn::SetInitialRotation()
{
	TArray<AActor*> PlayerStartActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStartActors);

	const AActor* InitialPlayerStart = nullptr;
	float ClosestDistance = MAX_FLT;

	for (const auto PlayerStartActor : PlayerStartActors)
	{
		const float Distance = FVector::Distance(GetActorLocation(), PlayerStartActor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			InitialPlayerStart = PlayerStartActor;
		}
	}

	if (Controller)
	{
		Controller->SetControlRotation(InitialPlayerStart->GetActorRotation());
	}
}
