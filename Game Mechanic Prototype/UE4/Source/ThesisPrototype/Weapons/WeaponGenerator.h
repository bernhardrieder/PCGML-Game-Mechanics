// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartedWeaponGeneratorEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGeneratorIsReadyEvent, bool, IsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponGenerationFinishedEvent, class AShooterWeapon*, GeneratedWeapon);


class AShooterWeapon;
enum class EWeaponType : uint8;
enum class EFireMode : uint8;
struct FWeaponStatistics;

USTRUCT(BlueprintType)
struct FWeaponGeneratorAPIJsonData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString damages_first;

	UPROPERTY(BlueprintReadWrite)
	FString damages_last;

	UPROPERTY(BlueprintReadWrite)
	FString distances_first;

	UPROPERTY(BlueprintReadWrite)
	FString distances_last;

	UPROPERTY(BlueprintReadWrite)
	FString firemode_Automatic;

	UPROPERTY(BlueprintReadWrite)
	FString firemode_Semi;

	UPROPERTY(BlueprintReadWrite)
	FString firemode_Single;

	UPROPERTY(BlueprintReadWrite)
	FString hiprecoildec;

	UPROPERTY(BlueprintReadWrite)
	FString hiprecoilright;

	UPROPERTY(BlueprintReadWrite)
	FString hiprecoilup;

	UPROPERTY(BlueprintReadWrite)
	FString hipstandbasespreaddec;

	UPROPERTY(BlueprintReadWrite)
	FString hipstandbasespreadinc;

	UPROPERTY(BlueprintReadWrite)
	FString magsize;

	UPROPERTY(BlueprintReadWrite)
	FString reloadempty;

	UPROPERTY(BlueprintReadWrite)
	FString rof;

	UPROPERTY(BlueprintReadWrite)
	FString shotspershell;

	UPROPERTY(BlueprintReadWrite)
	FString type_Pistol;

	UPROPERTY(BlueprintReadWrite)
	FString type_Rifle;

	UPROPERTY(BlueprintReadWrite)
	FString type_Shotgun;

	UPROPERTY(BlueprintReadWrite)
	FString type_Sniper;

	UPROPERTY(BlueprintReadWrite)
	FString type_SMG;

	UPROPERTY(BlueprintReadWrite)
	FString type_MG;
	
	UPROPERTY(BlueprintReadWrite)
	FString initial_speed;

	UPROPERTY(BlueprintReadWrite)
	FString success;

	FWeaponGeneratorAPIJsonData(){}
	FWeaponGeneratorAPIJsonData(FVector2D MaxDamageWithDistance, FVector2D MinDamageWithDistance, EWeaponType WeaponType, EFireMode FireMode,
		FVector2D RecoilIncreasePerShot, float RecoilDecrease, float BulletSpreadIncrease, float BulletSpreadDecrease, int32 RateOfFire, int32 BulletsPerMagazine,
		float ReloadTimeEmptyMagazine, int32 BulletsInOneShot, int32 MuzzleVelocity);
};

UCLASS()
class THESISPROTOTYPE_API AWeaponGenerator : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	pistolClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	sniperClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	machineGunClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	rifleClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	smgClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	shotgunClass;

	//distmantled weapon multiplier applied for generating a new weapon
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator|Dismanteld Random Modification")
	FVector2D randomModificationStartRange;

	//how much the range will be offset per kill
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator|Dismanteld Random Modification")
	float offsetPerKill;

	//how much the range will be offset per minute used
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator|Dismanteld Random Modification")
	float offsetPerMinuteUsed;

	//according to the highest found value among the types, how big should be the tolerance to include others and select a random one?
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator|Dismanteld Random Modification")
	float weaponTypeSelectionTolerance = 0.1f;

	//according to the highest found value among the types, how big should be the tolerance to include others and select a random one?
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator|Dismanteld Random Modification")
	float weaponFireModeSelectionTolerance = 0.1f;

public:
	AWeaponGenerator();

	//don't forget to subscribe to OnWeaponGenerationFinishedEvent to get notified when the new weapon is generated
	void DismantleWeapon(AShooterWeapon* Weapon);

	UPROPERTY(BlueprintAssignable, Category = "Weapon Generator|Events")
	FOnGeneratorIsReadyEvent OnGeneratorIsReadyEvent;

	UPROPERTY(BlueprintAssignable, Category = "Weapon Generator|Events")
	FOnStartedWeaponGeneratorEvent OnStartedWeaponGeneratorEvent;

	UPROPERTY(BlueprintAssignable, Category = "Weapon Generator|Events")
	FOnWeaponGenerationFinishedEvent OnWeaponGenerationFinishedEvent;

	FORCEINLINE bool IsGenerating() const {	return bIsGenerating; }
	FORCEINLINE bool IsReadyToUse() const { return bIsReadyToUse; }

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon Generator")
	void sendDismantledWeaponToGenerator(const FWeaponGeneratorAPIJsonData& JsonData);

	UFUNCTION(BlueprintCallable, Category = "Weapon Generator")
	void receiveNewWeaponFromGenerator(const FWeaponGeneratorAPIJsonData& JsonData);

	UFUNCTION(BlueprintCallable, Category = "Weapon Generator")
	void setReadyToUse(bool IsReady);

	FWeaponGeneratorAPIJsonData convertWeaponToJsonData(AShooterWeapon* Weapon);
	AShooterWeapon* constructWeaponFromJsonData(const FWeaponGeneratorAPIJsonData& JsonData);
	EWeaponType determineWeaponType(const FWeaponGeneratorAPIJsonData& JsonData);
	EFireMode determineWeaponFireMode(const FWeaponGeneratorAPIJsonData& JsonData);
	void applySomeModifications(AShooterWeapon* Weapon, FVector2D& MaxDamageWithDistance, FVector2D& MinDamageWithDistance, FVector2D& RecoilIncreasePerShot, float& RecoilDecrease,
		float& BulletSpreadIncrease, float& BulletSpreadDecrease, int32& RateOfFire, int32& BulletsPerMagazine, float& ReloadTimeEmptyMagazine,	int32& BulletsInOneShot, int32& MuzzleVelocity);

private:
	FRandomStream randomNumberGenerator;
	bool bIsGenerating = false;
	bool bIsReadyToUse = false;
};
