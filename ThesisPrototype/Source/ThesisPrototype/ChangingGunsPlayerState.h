// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ChangingGunsPlayerState.generated.h"

class AShooterWeapon;

USTRUCT(BlueprintType)
struct FWeaponStatistics
{
	GENERATED_BODY()

	FWeaponStatistics()
	{
		Weapon = nullptr;
		Kills = 0;
		SecondsUsed = 0;
	}

	UPROPERTY(BlueprintReadOnly, Category = "FWeaponStatistics")
	AShooterWeapon* Weapon;
	
	UPROPERTY(BlueprintReadOnly, Category = "FWeaponStatistics")
	int32 Kills;

	UPROPERTY(BlueprintReadOnly, Category = "FWeaponStatistics")
	float SecondsUsed;
};
/**
 * 
 */
UCLASS()
class THESISPROTOTYPE_API AChangingGunsPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Player State")
	void AddKill(AShooterWeapon* weapon);

	UFUNCTION(BlueprintCallable, Category = "Player State")
	void AddUsageTime(AShooterWeapon* weapon, float secondsUsed);

	UFUNCTION(BlueprintCallable, Category = "Player State")
	FWeaponStatistics GetWeaponStats(AShooterWeapon* weapon);

	UFUNCTION(BlueprintCallable, Category = "Player State")
	void RemoveWeaponFromStatistics(AShooterWeapon* weapon);

private:
	int32 findWeaponStatsIndexInArray(AShooterWeapon* weapon);
	FWeaponStatistics& getWeaponStats(AShooterWeapon* weapon);

	TArray<FWeaponStatistics> m_weaponStats;
};
