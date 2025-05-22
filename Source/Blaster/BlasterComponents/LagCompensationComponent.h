// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "Containers/RingBuffer.h"
#include "LagCompensationComponent.generated.h"

#define MAX_SIM_TIME 2.f
#define SIM_FREQUENCY 30.f

class ABlasterPlayerController;

USTRUCT(BlueprintType)
struct FHitboxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FHitboxInformation> HitboxInfoMap;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bIsHeadshot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> Headshots;

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> Bodyshots;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& FramePackage, FColor Color) const;
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                                         const FVector_NetQuantize& HitLocation, float HitTime);
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters,
	                                                       const FVector_NetQuantize& TraceStart,
	                                                       const TArray<FVector_NetQuantize>& HitLocations,
	                                                       float HitTime);
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,
	                                                   const FVector_NetQuantize& TraceStart,
	                                                   const FVector_NetQuantize100& InitialVelocity, float HitTime);

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                        const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser);

	UFUNCTION(Server, Reliable)
	void ServerShotgunScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters,
	                               const FVector_NetQuantize& TraceStart,
	                               const TArray<FVector_NetQuantize>& HitLocations, float HitTime,
	                               AWeapon* DamageCauser);

	UFUNCTION(Server, Reliable)
	void ServerProjectileScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                                  const FVector_NetQuantize100& InitialVelocity, float HitTime,
	                                  AWeapon* DamageCauser);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& FramePackage);
	FFramePackage GetFramePackageToCheck(ABlasterCharacter* HitCharacter, float HitTime);
	FFramePackage InterpBetweenFramePackages(const FFramePackage& YoungerFramePackage,
	                                         const FFramePackage& OlderFramePackage, float HitTime);

	FServerSideRewindResult ConfirmHit(const FFramePackage& FramePackage, const FVector_NetQuantize& TraceStart,
	                                   const FVector_NetQuantize& HitLocation);
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
	                                                 const FVector_NetQuantize& TraceStart,
	                                                 const TArray<FVector_NetQuantize>& HitLocations);
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& FramePackage,
	                                             const FVector_NetQuantize& TraceStart,
	                                             const FVector_NetQuantize100& InitialVelocity, float HitTime);

	void CacheHitboxPositions(const ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveHitboxes(const ABlasterCharacter* HitCharacter, const FFramePackage& FramePackage,
	                  bool bResetCollision = false);
	void EnableCharacterMeshCollision(const ABlasterCharacter* InCharacter,
	                                  ECollisionEnabled::Type CollisionEnabled) const;

private:
	void SaveFrameHistory();
	float CalculateFrameHistoryLength() const;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> Character;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> PlayerController;

	TRingBuffer<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 2.f;
};
