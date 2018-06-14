// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "WeaponGenerator.h"
#include "ShooterWeapon.h"
#include "Engine/World.h"

FWeaponGeneratorAPIJsonData::FWeaponGeneratorAPIJsonData(FVector2D maxDamageWithDistance, FVector2D minDamageWithDistance, EWeaponType weaponType, 
	EFireMode fireMode, FVector2D recoilIncreasePerShot, float recoilDecrease, float bulletSpreadIncrease, float bulletSpreadDecrease, int32 rateOfFire, 
	int32 bulletsPerMagazine, float reloadTimeEmptyMagazine, int32 bulletsInOneShot)
{
	damages_first = FString::SanitizeFloat(maxDamageWithDistance.X, 2);
	damages_last = FString::SanitizeFloat(minDamageWithDistance.X, 2);
	dmg_distances_first = FString::SanitizeFloat(maxDamageWithDistance.Y, 2);
	dmg_distances_last = FString::SanitizeFloat(minDamageWithDistance.Y, 2);

	hiprecoildec = FString::SanitizeFloat(recoilDecrease, 2);
	hiprecoilright = FString::SanitizeFloat(recoilIncreasePerShot.X, 2);
	hiprecoilup = FString::SanitizeFloat(recoilIncreasePerShot.Y, 2);
	hipstandbasespreaddec = FString::SanitizeFloat(bulletSpreadDecrease, 2);
	hipstandbasespreadinc = FString::SanitizeFloat(bulletSpreadIncrease, 2);
	magsize = FString::FromInt(bulletsPerMagazine);
	reloadempty = FString::SanitizeFloat(reloadTimeEmptyMagazine, 2);
	rof = FString::FromInt(rateOfFire);
	shotspershell = FString::FromInt(bulletsInOneShot);


	//categorical one hot encoding
	const FString zero = "0";
	const FString one = "1";

	firemode_Automatic = zero;
	firemode_Semi = zero;
	switch(fireMode) 
	{ 
		case EFireMode::SingleFire: 
			firemode_Automatic = one;
			break;
		case EFireMode::SemiAutomatic: 
			firemode_Semi = one;
			break;
	}

	type_Pistol = zero;
	type_Rifle = zero;
	type_Shotgun = zero;
	type_Sniper = zero;
	type_SMG = zero;

	switch(weaponType)
	{
		case EWeaponType::Pistol: 
			type_Pistol = one;
			break;
		case EWeaponType::Shotgun: 
			type_Shotgun = one;
			break;
		case EWeaponType::SubMachineGun: 
			type_SMG = one;
			break;
		case EWeaponType::Rifle: 
			type_Rifle = one; 
			break;
		case EWeaponType::SniperRifle: 
			type_Sniper = one;
			break;
	}
}

AWeaponGenerator::AWeaponGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	CategoricalDataThreshold = 0.5;
	RandomSeed = 19071991;
}

void AWeaponGenerator::BeginPlay()
{
	Super::BeginPlay();
	m_randomNumberGenerator.Initialize(RandomSeed);
}

void AWeaponGenerator::DismantleWeapon(AShooterWeapon* weapon)
{
	sendDismantledWeaponToGenerator(convertWeaponToJsonData(weapon));
}

// this is just a stub implementation which is called if there is no implementation in BP
void AWeaponGenerator::sendDismantledWeaponToGenerator_Implementation(const FWeaponGeneratorAPIJsonData& jsonData)
{
	UE_LOG(LogTemp, Error, TEXT("No BP implementation for sendDismantledWeaponToGenerator function in weapon generator!!"))
}

void AWeaponGenerator::receiveNewWeaponFromGenerator(const FWeaponGeneratorAPIJsonData& jsonData)
{
	AShooterWeapon* weapon = constructWeaponFromJsonData(jsonData);
	if(weapon)
	{
		OnWeaponGenerationReadyEvent.Broadcast(weapon);
	}
}

FWeaponGeneratorAPIJsonData AWeaponGenerator::convertWeaponToJsonData(AShooterWeapon* weapon)
{
	const FVector2D maxDamageWithDistance = weapon->GetMaxDamageWithDistance();
	const FVector2D minDamageWithDistance = weapon->GetMinDamageWithDistance();
	const EWeaponType weaponType = weapon->GetType();
	const EFireMode fireMode = weapon->GetFireMode();
	const FVector2D recoilIncreasePerShot = weapon->GetRecoilIncreasePerShot();
	const float recoilDecrease = weapon->GetRecoilDecrease();
	const float bulletSpreadIncrease = weapon->GetBulletSpreadIncrease();
	const float bulletSpreadDecrease = weapon->GetBulletSpreadDecrease();
	const int32 rateOfFire = weapon->GetRateOfFire();
	const int32 bulletsPerMagazine = weapon->GetBulletsPerMagazine();
	const float reloadTimeEmptyMagazine = weapon->GetReloadTimeEmptyMagazine();
	const int32 bulletsInOneShot = weapon->GetBulletsInOneShot();

	return FWeaponGeneratorAPIJsonData(maxDamageWithDistance, minDamageWithDistance, weaponType,
		fireMode, recoilIncreasePerShot, recoilDecrease, bulletSpreadIncrease, bulletSpreadDecrease, rateOfFire,
		bulletsPerMagazine, reloadTimeEmptyMagazine, bulletsInOneShot);

}

AShooterWeapon* AWeaponGenerator::constructWeaponFromJsonData(const FWeaponGeneratorAPIJsonData& jsonData) const
{
	TSubclassOf<AShooterWeapon> weaponClass;
	switch(determineWeaponType(jsonData))
	{
	case EWeaponType::Pistol: weaponClass = PistolClass; break;
	case EWeaponType::Shotgun: weaponClass = ShotgunClass; break;
	case EWeaponType::SubMachineGun: weaponClass = SmgClass; break;
	case EWeaponType::Rifle: weaponClass = RifleClass; break;
	case EWeaponType::SniperRifle: weaponClass = SniperClass; break;
	case EWeaponType::HeavyMachineGun: weaponClass = MachineGunClass;  break;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShooterWeapon* weapon = GetWorld()->SpawnActor<AShooterWeapon>(weaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);

	if (!weapon)
		return nullptr;
	
	weapon->SetMaxDamageWithDistance(
		FVector2D(
			FCString::Atof(*jsonData.damages_first), 
			FCString::Atof(*jsonData.dmg_distances_first)
		));
	weapon->SetMinDamageWithDistance(
		FVector2D(
			FCString::Atof(*jsonData.damages_last),
			FCString::Atof(*jsonData.dmg_distances_last)
		));
	weapon->SetRecoilIncreasePerShot(
		FVector2D(
			FCString::Atof(*jsonData.hiprecoilright),
			FCString::Atof(*jsonData.hiprecoilup)
		));
	weapon->SetRecoilDecrease(FCString::Atof(*jsonData.hiprecoildec));
	weapon->SetBulletSpreadIncrease(FCString::Atof(*jsonData.hipstandbasespreadinc));
	weapon->SetBulletSpreadDecrease(FCString::Atof(*jsonData.hipstandbasespreaddec));
	weapon->SetBulletsPerMagazine(FCString::Atoi(*jsonData.magsize));
	weapon->SetReloadTimeEmptyMagazine(FCString::Atof(*jsonData.reloadempty));
	weapon->SetRateOfFire(FCString::Atoi(*jsonData.rof));
	weapon->SetBulletsInOneShot(FCString::Atoi(*jsonData.shotspershell));
	weapon->SetFireMode(determineWeaponFireMode(jsonData));

	return weapon;
}

EWeaponType AWeaponGenerator::determineWeaponType(const FWeaponGeneratorAPIJsonData& jsonData) const
{
	//determine the type first and then generate the weapon based on a base class
	float typePistol = FCString::Atof(*jsonData.type_Pistol);
	float typeSniper = FCString::Atof(*jsonData.type_Sniper);
	float typeRifle = FCString::Atof(*jsonData.type_Rifle);
	float typeSmg = FCString::Atof(*jsonData.type_SMG);
	float typeShotgun = FCString::Atof(*jsonData.type_Shotgun);

	float winnerFirstRound = FMath::Max3(typePistol, typeSniper, typeRifle);
	float winner = FMath::Max3(winnerFirstRound, typeSmg, typeShotgun);

	//just set it to rifle as default
	EWeaponType winnerType = EWeaponType::Rifle;
	
	if (FMath::IsNearlyEqual(typePistol, winner))
	{
		winnerType = EWeaponType::Pistol;
	}
	else if (FMath::IsNearlyEqual(typeSniper, winner))
	{
		winnerType = EWeaponType::SniperRifle;
	}
	else if(FMath::IsNearlyEqual(typeRifle, winner))
	{
		winnerType = EWeaponType::Rifle;
	}
	else if (FMath::IsNearlyEqual(typeSmg, winner))
	{
		winnerType = EWeaponType::SubMachineGun;
	}
	else if (FMath::IsNearlyEqual(typeShotgun, winner))
	{
		winnerType = EWeaponType::Shotgun;
	}

	EWeaponType type = winnerType;
	if(winner <= CategoricalDataThreshold)
	{
		type = m_randomNumberGenerator.FRandRange(0.0f, 1.0f) >= 0.5f ? winnerType : EWeaponType::HeavyMachineGun;
	}

	return type;
}

EFireMode AWeaponGenerator::determineWeaponFireMode(const FWeaponGeneratorAPIJsonData& jsonData) const
{
	float fireModeAutomatic = FCString::Atof(*jsonData.firemode_Automatic);
	float fireModeSemi = FCString::Atof(*jsonData.firemode_Semi);

	float winner = FMath::Max(fireModeAutomatic, fireModeSemi);

	//just set it to automatic as default
	EFireMode winnerFireMode = EFireMode::Automatic;
	if (FMath::IsNearlyEqual(fireModeAutomatic, winner))
	{
		winnerFireMode = EFireMode::Automatic;
	}
	else if (FMath::IsNearlyEqual(fireModeSemi, winner))
	{
		winnerFireMode = EFireMode::SemiAutomatic;
	}

	EFireMode firemode = winnerFireMode;
	if (winner <= CategoricalDataThreshold)
	{
		firemode = m_randomNumberGenerator.FRandRange(0.0f, 1.0f) >= 0.5f ? winnerFireMode : EFireMode::SingleFire;
	}

	return firemode;
}
