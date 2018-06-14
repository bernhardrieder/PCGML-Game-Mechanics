// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "WeaponGenerator.h"
#include "ShooterWeapon.h"
#include "Engine/World.h"
#include "ChangingGuns.h"

FWeaponGeneratorAPIJsonData::FWeaponGeneratorAPIJsonData(FVector2D maxDamageWithDistance, FVector2D minDamageWithDistance, EWeaponType weaponType,
	EFireMode fireMode, FVector2D recoilIncreasePerShot, float recoilDecrease, float bulletSpreadIncrease, float bulletSpreadDecrease, int32 rateOfFire,
	int32 bulletsPerMagazine, float reloadTimeEmptyMagazine, int32 bulletsInOneShot, int32 muzzleVelocity)
{
	damages_first = FString::SanitizeFloat(maxDamageWithDistance.X, 4);
	damages_last = FString::SanitizeFloat(minDamageWithDistance.X, 4) ;
	distances_first = FString::SanitizeFloat(maxDamageWithDistance.Y / PROJECT_MEASURING_UNIT_FACTOR_TO_M, 4);
	distances_last = FString::SanitizeFloat(minDamageWithDistance.Y / PROJECT_MEASURING_UNIT_FACTOR_TO_M, 4);

	hiprecoildec = FString::SanitizeFloat(recoilDecrease, 4);
	hiprecoilright = FString::SanitizeFloat(recoilIncreasePerShot.X, 4);
	hiprecoilup = FString::SanitizeFloat(recoilIncreasePerShot.Y, 4);
	hipstandbasespreaddec = FString::SanitizeFloat(bulletSpreadDecrease, 4);
	hipstandbasespreadinc = FString::SanitizeFloat(bulletSpreadIncrease, 4);
	magsize = FString::FromInt(bulletsPerMagazine);
	reloadempty = FString::SanitizeFloat(reloadTimeEmptyMagazine, 4);
	rof = FString::FromInt(rateOfFire);
	shotspershell = FString::FromInt(bulletsInOneShot);
	initialspeed = FString::FromInt(muzzleVelocity);


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
	success = "hopefully :P";
}

AWeaponGenerator::AWeaponGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	MinCategoricalDataThreshold = 0.5;

	RandomModificationStartRange = FVector2D(0.8f, 1.2f);
	OffsetPerKill = 0.05f;
	OffsetPerMinuteUsed = 0.1f;
}

void AWeaponGenerator::BeginPlay()
{
	Super::BeginPlay();

}

void AWeaponGenerator::DismantleWeapon(AShooterWeapon* weapon)
{
	m_bIsGenerating = true;
	OnStartedWeaponGeneratorEvent.Broadcast();
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
		OnWeaponGenerationFinishedEvent.Broadcast(weapon);
	}
	m_bIsGenerating = false;
}

void AWeaponGenerator::setReadyToUse(bool isReady)
{
	m_IsReadyToUse = isReady;
	OnGeneratorIsReadyEvent.Broadcast(isReady);
}

FWeaponGeneratorAPIJsonData AWeaponGenerator::convertWeaponToJsonData(AShooterWeapon* weapon)
{
	FVector2D maxDamageWithDistance = weapon->GetMaxDamageWithDistance();
	FVector2D minDamageWithDistance = weapon->GetMinDamageWithDistance();
	const EWeaponType weaponType = weapon->GetType();
	const EFireMode fireMode = weapon->GetFireMode();
	FVector2D recoilIncreasePerShot = weapon->GetRecoilIncreasePerShot();
	float recoilDecrease = weapon->GetRecoilDecrease();
	float bulletSpreadIncrease = weapon->GetBulletSpreadIncrease();
	float bulletSpreadDecrease = weapon->GetBulletSpreadDecrease();
	int32 rateOfFire = weapon->GetRateOfFire();
	int32 bulletsPerMagazine = weapon->GetBulletsPerMagazine();
	float reloadTimeEmptyMagazine = weapon->GetReloadTimeEmptyMagazine();
	int32 bulletsInOneShot = weapon->GetBulletsInOneShot();
	int32 muzzleVelocity = weapon->GetMuzzleVelocity();

	applySomeModifications(weapon, maxDamageWithDistance, minDamageWithDistance, recoilIncreasePerShot, recoilDecrease, bulletSpreadIncrease, bulletSpreadDecrease, rateOfFire,
		bulletsPerMagazine, reloadTimeEmptyMagazine, bulletsInOneShot, muzzleVelocity);

	return FWeaponGeneratorAPIJsonData(maxDamageWithDistance, minDamageWithDistance, weaponType,
		fireMode, recoilIncreasePerShot, recoilDecrease, bulletSpreadIncrease, bulletSpreadDecrease, rateOfFire,
		bulletsPerMagazine, reloadTimeEmptyMagazine, bulletsInOneShot, muzzleVelocity);

}

AShooterWeapon* AWeaponGenerator::constructWeaponFromJsonData(const FWeaponGeneratorAPIJsonData& jsonData)
{
	if (!jsonData.success.Equals("true"))
		return nullptr;

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

	if (!weaponClass.GetDefaultObject())
		return nullptr;

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShooterWeapon* weapon = GetWorld()->SpawnActor<AShooterWeapon>(weaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);

	if (!weapon)
		return nullptr;

	weapon->SetFireMode(determineWeaponFireMode(jsonData));

	weapon->SetMaxDamageWithDistance(
		FVector2D(
			FCString::Atof(*jsonData.damages_first),
			FMath::Max(0.f, FCString::Atof(*jsonData.distances_first) * PROJECT_MEASURING_UNIT_FACTOR_TO_M)
		));
	weapon->SetMinDamageWithDistance(
		FVector2D(
			FCString::Atof(*jsonData.damages_last),
			FCString::Atof(*jsonData.distances_last) * PROJECT_MEASURING_UNIT_FACTOR_TO_M
		));

	//the clamping should fix the recoil bugs
	const float recoilIncreaseRight = FMath::Clamp(FCString::Atof(*jsonData.hiprecoilright), 0.f, 20.f);
	const float recoilIncreaseUp = FMath::Clamp(FCString::Atof(*jsonData.hiprecoilup), 0.f, 20.f);
	weapon->SetRecoilIncreasePerShot(
		FVector2D(
			recoilIncreaseRight,
			recoilIncreaseUp
		));
	weapon->SetRecoilDecrease(FMath::Clamp(FCString::Atof(*jsonData.hiprecoildec), 0.f, 7.5f));

	const float bulletSpreadIncrease = FMath::Max(0.f, FCString::Atof(*jsonData.hipstandbasespreadinc));
	weapon->SetBulletSpreadIncrease(bulletSpreadIncrease);
	weapon->SetBulletSpreadDecrease(FMath::Clamp(FCString::Atof(*jsonData.hipstandbasespreaddec), 0.f, bulletSpreadIncrease*10.f));
	
	int32 magSize = FCString::Atoi(*jsonData.magsize);
	magSize *= magSize < 0 ? -1 : 1;
	weapon->SetBulletsPerMagazine(magSize);
	
	weapon->SetReloadTimeEmptyMagazine(FMath::Max(0.f, FCString::Atof(*jsonData.reloadempty)));
	weapon->SetBulletsInOneShot(FMath::Max(1, FCString::Atoi(*jsonData.shotspershell)));

	weapon->SetRateOfFire(FCString::Atoi(*jsonData.rof));
	weapon->SetMuzzleVelocity(FCString::Atoi(*jsonData.initialspeed));

	return weapon;
}

EWeaponType AWeaponGenerator::determineWeaponType(const FWeaponGeneratorAPIJsonData& jsonData)
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

	float tolerance = 0.01f;
	if (FMath::IsNearlyEqual(typePistol, winner, tolerance))
	{
		winnerType = EWeaponType::Pistol;
	}
	else if (FMath::IsNearlyEqual(typeSniper, winner, tolerance))
	{
		winnerType = EWeaponType::SniperRifle;
	}
	else if(FMath::IsNearlyEqual(typeRifle, winner, tolerance))
	{
		winnerType = EWeaponType::Rifle;
	}
	else if (FMath::IsNearlyEqual(typeSmg, winner, tolerance))
	{
		winnerType = EWeaponType::SubMachineGun;
	}
	else if (FMath::IsNearlyEqual(typeShotgun, winner, tolerance))
	{
		winnerType = EWeaponType::Shotgun;
	}

	EWeaponType type = winnerType;
	if(winner <= MinCategoricalDataThreshold)
	{
		m_randomNumberGenerator.GenerateNewSeed();
		type = m_randomNumberGenerator.FRandRange(0.0f, 100.0f) >= 50.f ? winnerType : EWeaponType::HeavyMachineGun;
	}

	return type;
}

EFireMode AWeaponGenerator::determineWeaponFireMode(const FWeaponGeneratorAPIJsonData& jsonData)
{
	float fireModeAutomatic = FCString::Atof(*jsonData.firemode_Automatic);
	float fireModeSemi = FCString::Atof(*jsonData.firemode_Semi);

	float winner = FMath::Max(fireModeAutomatic, fireModeSemi);

	//just set it to automatic as default
	EFireMode winnerFireMode = EFireMode::Automatic;
	float tolerance = 0.01f;
	if (FMath::IsNearlyEqual(fireModeAutomatic, winner, tolerance))
	{
		winnerFireMode = EFireMode::Automatic;
	}
	else if (FMath::IsNearlyEqual(fireModeSemi, winner, tolerance))
	{
		winnerFireMode = EFireMode::SemiAutomatic;
	}

	EFireMode firemode = winnerFireMode;
	if (winner <= MinCategoricalDataThreshold)
	{
		m_randomNumberGenerator.GenerateNewSeed();
		firemode = m_randomNumberGenerator.FRandRange(0.0f, 100.0f) >= 50.f ? winnerFireMode : EFireMode::SingleFire;
	}

	return firemode;
}

void AWeaponGenerator::applySomeModifications(AShooterWeapon* weapon, FVector2D& maxDamageWithDistance, FVector2D& minDamageWithDistance, FVector2D& recoilIncreasePerShot, float& recoilDecrease, 
	float& bulletSpreadIncrease, float& bulletSpreadDecrease, int32& rateOfFire, int32& bulletsPerMagazine, float& reloadTimeEmptyMagazine, int32& bulletsInOneShot, int32& muzzleVelocity)
{
	m_randomNumberGenerator.GenerateNewSeed();
	FVector2D increaseRandomRange = RandomModificationStartRange;
	FVector2D decreaseRandomRange = RandomModificationStartRange;

	//offset the range regarding the stats
	const FWeaponStatistics statistics = weapon->GetWeaponStatistics();
	const float offset = (statistics.Kills * OffsetPerKill) + (FMath::FloorToInt(statistics.SecondsUsed/60.f)*OffsetPerMinuteUsed);
	increaseRandomRange += FVector2D(offset, offset);
	decreaseRandomRange -= FVector2D(offset, offset);

	//damage
	maxDamageWithDistance.X *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);
	minDamageWithDistance.X *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	//distance
	maxDamageWithDistance.Y *= m_randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y); 
	minDamageWithDistance.Y *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	recoilIncreasePerShot.X *= m_randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);
	recoilIncreasePerShot.Y *= m_randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);

	recoilDecrease *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	bulletSpreadIncrease *= m_randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);

	bulletSpreadDecrease *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	rateOfFire *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	bulletsPerMagazine *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	reloadTimeEmptyMagazine *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	bulletsInOneShot *= 1;

	muzzleVelocity *= m_randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);
}