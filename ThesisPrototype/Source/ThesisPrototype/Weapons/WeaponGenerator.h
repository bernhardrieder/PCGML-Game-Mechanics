// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponGenerationReadyEvent, class AShooterWeapon*, generatedWeapon);

class AShooterWeapon;
enum class EWeaponType : uint8;
enum class EFireMode : uint8;

USTRUCT(BlueprintType)
struct FWeaponGeneratorAPIJsonData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString damages_first;

	UPROPERTY(BlueprintReadWrite)
	FString damages_last;

	UPROPERTY(BlueprintReadWrite)
	FString dmg_distances_first;

	UPROPERTY(BlueprintReadWrite)
	FString dmg_distances_last;

	UPROPERTY(BlueprintReadWrite)
	FString firemode_Automatic;

	UPROPERTY(BlueprintReadWrite)
	FString firemode_Semi;

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


	FWeaponGeneratorAPIJsonData() {};
	FWeaponGeneratorAPIJsonData(FVector2D maxDamageWithDistance, FVector2D minDamageWithDistance, EWeaponType weaponType, EFireMode fireMode,
		FVector2D recoilIncreasePerShot, float recoilDecrease, float bulletSpreadIncrease, float bulletSpreadDecrease, int32 rateOfFire, int32 bulletsPerMagazine,
		float reloadTimeEmptyMagazine, int32 bulletsInOneShot);
};

UCLASS()
class THESISPROTOTYPE_API AWeaponGenerator : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	PistolClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	SniperClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	MachineGunClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	RifleClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	SmgClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	TSubclassOf<AShooterWeapon>	ShotgunClass;

	//this is the threshold which is checked for categorical data. if the value is above the threshold then it determines its firemode or type otherwise its randomized
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	float CategoricalDataThreshold;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Generator")
	int32 RandomSeed;

public:
	AWeaponGenerator();

	//don't forget to subscribe to OnWeaponGenerationReadyEvent to get notified when the new weapon is generated
	void DismantleWeapon(AShooterWeapon* weapon);

	FOnWeaponGenerationReadyEvent OnWeaponGenerationReadyEvent;

	FORCEINLINE bool IsGenerating() const {	return m_bIsGenerating;	}

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon Generator")
	void sendDismantledWeaponToGenerator(const FWeaponGeneratorAPIJsonData& jsonData);

	UFUNCTION(BlueprintCallable, Category = "Weapon Generator")
	void receiveNewWeaponFromGenerator(const FWeaponGeneratorAPIJsonData& jsonData);

	static FWeaponGeneratorAPIJsonData convertWeaponToJsonData(AShooterWeapon* weapon);
	AShooterWeapon* constructWeaponFromJsonData(const FWeaponGeneratorAPIJsonData& jsonData) const;
	EWeaponType determineWeaponType(const FWeaponGeneratorAPIJsonData& jsonData) const;
	EFireMode determineWeaponFireMode(const FWeaponGeneratorAPIJsonData& jsonData) const;

private:
	FRandomStream m_randomNumberGenerator;
	bool m_bIsGenerating = false;
};
