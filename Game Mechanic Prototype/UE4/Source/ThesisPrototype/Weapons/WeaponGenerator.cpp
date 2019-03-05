// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "WeaponGenerator.h"
#include "ShooterWeapon.h"
#include "Engine/World.h"
#include "ChangingGuns.h"

FWeaponGeneratorAPIJsonData::FWeaponGeneratorAPIJsonData(FVector2D MaxDamageWithDistance, FVector2D MinDamageWithDistance, EWeaponType WeaponType,
	EFireMode FireMode, FVector2D RecoilIncreasePerShot, float RecoilDecrease, float BulletSpreadIncrease, float BulletSpreadDecrease, int32 RateOfFire,
	int32 BulletsPerMagazine, float ReloadTimeEmptyMagazine, int32 BulletsInOneShot, int32 MuzzleVelocity)
{
	damages_first = FString::SanitizeFloat(MaxDamageWithDistance.X, 4);
	damages_last = FString::SanitizeFloat(MinDamageWithDistance.X, 4) ;
	distances_first = FString::SanitizeFloat(MaxDamageWithDistance.Y / PROJECT_MEASURING_UNIT_FACTOR_TO_M, 4);
	distances_last = FString::SanitizeFloat(MinDamageWithDistance.Y / PROJECT_MEASURING_UNIT_FACTOR_TO_M, 4);

	hiprecoildec = FString::SanitizeFloat(RecoilDecrease, 4);
	hiprecoilright = FString::SanitizeFloat(RecoilIncreasePerShot.X, 4);
	hiprecoilup = FString::SanitizeFloat(RecoilIncreasePerShot.Y, 4);
	hipstandbasespreaddec = FString::SanitizeFloat(BulletSpreadDecrease, 4);
	hipstandbasespreadinc = FString::SanitizeFloat(BulletSpreadIncrease, 4);
	magsize = FString::FromInt(BulletsPerMagazine);
	reloadempty = FString::SanitizeFloat(ReloadTimeEmptyMagazine, 4);
	rof = FString::FromInt(RateOfFire);
	shotspershell = FString::FromInt(BulletsInOneShot);
	initial_speed = FString::FromInt(MuzzleVelocity);


	//categorical one hot encoding
	const FString zero = "0";
	const FString one = "1";

	firemode_Automatic = zero;
	firemode_Semi = zero;
	firemode_Single = zero;
	switch(FireMode)
	{
		case EFireMode::Automatic:
			firemode_Automatic = one;
			break;
		case EFireMode::SemiAutomatic:
			firemode_Semi = one;
			break;
		case EFireMode::SingleFire:
			firemode_Single = one;
			break;
	}

	type_Pistol = zero;
	type_Rifle = zero;
	type_Shotgun = zero;
	type_Sniper = zero;
	type_SMG = zero;
	type_MG = zero;

	switch(WeaponType)
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
		case EWeaponType::HeavyMachineGun:
			type_MG = one;
			break;
	}
	success = "hopefully :P";
}

AWeaponGenerator::AWeaponGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	randomModificationStartRange = FVector2D(0.8f, 1.2f);
	offsetPerKill = 0.05f;
	offsetPerMinuteUsed = 0.1f;
}

void AWeaponGenerator::DismantleWeapon(AShooterWeapon* Weapon)
{
	bIsGenerating = true;
	OnStartedWeaponGeneratorEvent.Broadcast();
	sendDismantledWeaponToGenerator(convertWeaponToJsonData(Weapon));
}

// this is just a stub implementation which is called if there is no implementation in BP
void AWeaponGenerator::sendDismantledWeaponToGenerator_Implementation(const FWeaponGeneratorAPIJsonData& JsonData)
{
	UE_LOG(LogTemp, Error, TEXT("No BP implementation for sendDismantledWeaponToGenerator function in weapon generator!!"))
}

void AWeaponGenerator::receiveNewWeaponFromGenerator(const FWeaponGeneratorAPIJsonData& JsonData)
{
	AShooterWeapon* weapon = constructWeaponFromJsonData(JsonData);
	if(weapon)
	{
		OnWeaponGenerationFinishedEvent.Broadcast(weapon);
	}
	bIsGenerating = false;
}

void AWeaponGenerator::setReadyToUse(bool IsReady)
{
	bIsReadyToUse = IsReady;
	OnGeneratorIsReadyEvent.Broadcast(IsReady);
}

FWeaponGeneratorAPIJsonData AWeaponGenerator::convertWeaponToJsonData(AShooterWeapon* Weapon)
{
	FVector2D maxDamageWithDistance = Weapon->GetMaxDamageWithDistance();
	FVector2D minDamageWithDistance = Weapon->GetMinDamageWithDistance();
	const EWeaponType weaponType = Weapon->GetType();
	const EFireMode fireMode = Weapon->GetFireMode();
	FVector2D recoilIncreasePerShot = Weapon->GetRecoilIncreasePerShot();
	float recoilDecrease = Weapon->GetRecoilDecrease();
	float bulletSpreadIncrease = Weapon->GetBulletSpreadIncrease();
	float bulletSpreadDecrease = Weapon->GetBulletSpreadDecrease();
	int32 rateOfFire = Weapon->GetRateOfFire();
	int32 bulletsPerMagazine = Weapon->GetBulletsPerMagazine();
	float reloadTimeEmptyMagazine = Weapon->GetReloadTimeEmptyMagazine();
	int32 bulletsInOneShot = Weapon->GetBulletsInOneShot();
	int32 muzzleVelocity = Weapon->GetMuzzleVelocity();

	applySomeModifications(Weapon, maxDamageWithDistance, minDamageWithDistance, recoilIncreasePerShot, recoilDecrease, bulletSpreadIncrease, bulletSpreadDecrease, rateOfFire,
		bulletsPerMagazine, reloadTimeEmptyMagazine, bulletsInOneShot, muzzleVelocity);

	return FWeaponGeneratorAPIJsonData(maxDamageWithDistance, minDamageWithDistance, weaponType,
		fireMode, recoilIncreasePerShot, recoilDecrease, bulletSpreadIncrease, bulletSpreadDecrease, rateOfFire,
		bulletsPerMagazine, reloadTimeEmptyMagazine, bulletsInOneShot, muzzleVelocity);

}

AShooterWeapon* AWeaponGenerator::constructWeaponFromJsonData(const FWeaponGeneratorAPIJsonData& JsonData)
{
	if (!JsonData.success.Equals("true"))
		return nullptr;

	TSubclassOf<AShooterWeapon> weaponClass;
	switch(determineWeaponType(JsonData))
	{
	case EWeaponType::Pistol: weaponClass = pistolClass; break;
	case EWeaponType::Shotgun: weaponClass = shotgunClass; break;
	case EWeaponType::SubMachineGun: weaponClass = smgClass; break;
	case EWeaponType::Rifle: weaponClass = rifleClass; break;
	case EWeaponType::SniperRifle: weaponClass = sniperClass; break;
	case EWeaponType::HeavyMachineGun: weaponClass = machineGunClass;  break;
	}

	if (!weaponClass.GetDefaultObject())
		return nullptr;

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AShooterWeapon* weapon = GetWorld()->SpawnActor<AShooterWeapon>(weaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);

	if (!weapon)
		return nullptr;

	weapon->SetFireMode(determineWeaponFireMode(JsonData));

	weapon->SetMaxDamageWithDistance(
		FVector2D(
			FCString::Atof(*JsonData.damages_first),
			FMath::Max(0.f, FCString::Atof(*JsonData.distances_first) * PROJECT_MEASURING_UNIT_FACTOR_TO_M)
		));
	weapon->SetMinDamageWithDistance(
		FVector2D(
			FCString::Atof(*JsonData.damages_last),
			FCString::Atof(*JsonData.distances_last) * PROJECT_MEASURING_UNIT_FACTOR_TO_M
		));

	//the clamping should fix the recoil bugs
	const float recoilIncreaseRight = FMath::Clamp(FCString::Atof(*JsonData.hiprecoilright), 0.f, 20.f);
	const float recoilIncreaseUp = FMath::Clamp(FCString::Atof(*JsonData.hiprecoilup), 0.f, 20.f);
	weapon->SetRecoilIncreasePerShot(
		FVector2D(
			recoilIncreaseRight,
			recoilIncreaseUp
		));
	weapon->SetRecoilDecrease(FMath::Clamp(FCString::Atof(*JsonData.hiprecoildec), 0.f, 7.5f));

	const float bulletSpreadIncrease = FMath::Max(0.f, FCString::Atof(*JsonData.hipstandbasespreadinc));
	weapon->SetBulletSpreadIncrease(bulletSpreadIncrease);
	weapon->SetBulletSpreadDecrease(FMath::Clamp(FCString::Atof(*JsonData.hipstandbasespreaddec), 0.f, bulletSpreadIncrease*10.f));
	
	int32 magSize = FCString::Atoi(*JsonData.magsize);
	magSize *= magSize < 0 ? -1 : 1;
	weapon->SetBulletsPerMagazine(FMath::Max(1, magSize));
	
	weapon->SetReloadTimeEmptyMagazine(FMath::Max(0.f, FCString::Atof(*JsonData.reloadempty)));
	weapon->SetBulletsInOneShot(FMath::Max(1, FCString::Atoi(*JsonData.shotspershell)));

	weapon->SetRateOfFire(FCString::Atoi(*JsonData.rof));
	weapon->SetMuzzleVelocity(FCString::Atoi(*JsonData.initial_speed));

	return weapon;
}

EWeaponType AWeaponGenerator::determineWeaponType(const FWeaponGeneratorAPIJsonData& JsonData)
{
	//determine the type first and then generate the weapon based on a base class
	float typePistol = FCString::Atof(*JsonData.type_Pistol);
	float typeSniper = FCString::Atof(*JsonData.type_Sniper);
	float typeRifle = FCString::Atof(*JsonData.type_Rifle);
	float typeSmg = FCString::Atof(*JsonData.type_SMG);
	float typeShotgun = FCString::Atof(*JsonData.type_Shotgun);
	float typeMg = FCString::Atof(*JsonData.type_MG);

	float winnerFirstRound = FMath::Max3(typePistol, typeSniper, typeRifle);
	float winnerSecondRound = FMath::Max3(winnerFirstRound, typeSmg, typeShotgun);
	float winner = FMath::Max(winnerSecondRound, typeMg);


	TArray<EWeaponType> winnerTypes;
	if (FMath::IsNearlyEqual(typePistol, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::Pistol);
	}
	if (FMath::IsNearlyEqual(typeSniper, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::SniperRifle);
	}
	if(FMath::IsNearlyEqual(typeRifle, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::Rifle);
	}
	if (FMath::IsNearlyEqual(typeSmg, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::SubMachineGun);
	}
	if (FMath::IsNearlyEqual(typeShotgun, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::Shotgun);
	} 
	if (FMath::IsNearlyEqual(typeMg, winner, weaponTypeSelectionTolerance))
	{
		winnerTypes.Add(EWeaponType::HeavyMachineGun);
	}

	if(winnerTypes.Num() > 0)
	{
		randomNumberGenerator.GenerateNewSeed();
		return winnerTypes[randomNumberGenerator.RandRange(0, winnerTypes.Num() - 1)];
	}

	return EWeaponType::Rifle;
}

EFireMode AWeaponGenerator::determineWeaponFireMode(const FWeaponGeneratorAPIJsonData& JsonData)
{
	float fireModeAutomatic = FCString::Atof(*JsonData.firemode_Automatic);
	float fireModeSemi = FCString::Atof(*JsonData.firemode_Semi);
	float fireModeSingle = FCString::Atof(*JsonData.firemode_Single);

	float winner = FMath::Max3(fireModeAutomatic, fireModeSemi, fireModeSingle);

	TArray<EFireMode> winnerFireModes;
	if (FMath::IsNearlyEqual(fireModeAutomatic, winner, weaponFireModeSelectionTolerance))
	{
		winnerFireModes.Add(EFireMode::Automatic);
	}
	if (FMath::IsNearlyEqual(fireModeSemi, winner, weaponFireModeSelectionTolerance))
	{
		winnerFireModes.Add(EFireMode::SemiAutomatic);
	}
	if (FMath::IsNearlyEqual(fireModeSingle, winner, weaponFireModeSelectionTolerance))
	{
		winnerFireModes.Add(EFireMode::SingleFire);
	}

	if (winnerFireModes.Num() > 0)
	{
		randomNumberGenerator.GenerateNewSeed();
		return winnerFireModes[randomNumberGenerator.RandRange(0, winnerFireModes.Num() - 1)];
	}

	return EFireMode::Automatic;;
}

void AWeaponGenerator::applySomeModifications(AShooterWeapon* Weapon, FVector2D& MaxDamageWithDistance, FVector2D& MinDamageWithDistance, FVector2D& RecoilIncreasePerShot, float& RecoilDecrease, 
	float& BulletSpreadIncrease, float& BulletSpreadDecrease, int32& RateOfFire, int32& BulletsPerMagazine, float& ReloadTimeEmptyMagazine, int32& BulletsInOneShot, int32& MuzzleVelocity)
{
	randomNumberGenerator.GenerateNewSeed();
	FVector2D increaseRandomRange = randomModificationStartRange;
	FVector2D decreaseRandomRange = randomModificationStartRange;

	//offset the range regarding the stats
	const FWeaponStatistics statistics = Weapon->GetWeaponStatistics();
	const float offset = (statistics.Kills * offsetPerKill) + (FMath::FloorToInt(statistics.SecondsUsed/60.f)*offsetPerMinuteUsed);
	increaseRandomRange += FVector2D(offset, offset);
	decreaseRandomRange -= FVector2D(offset, offset);

	//damage
	MaxDamageWithDistance.X *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);
	MinDamageWithDistance.X *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	//distance
	MaxDamageWithDistance.Y *= randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y); 
	MinDamageWithDistance.Y *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	RecoilIncreasePerShot.X *= randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);
	RecoilIncreasePerShot.Y *= randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);

	RecoilDecrease *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	BulletSpreadIncrease *= randomNumberGenerator.FRandRange(decreaseRandomRange.X, decreaseRandomRange.Y);

	BulletSpreadDecrease *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	RateOfFire *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	BulletsPerMagazine *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	ReloadTimeEmptyMagazine *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);

	BulletsInOneShot *= 1;

	MuzzleVelocity *= randomNumberGenerator.FRandRange(increaseRandomRange.X, increaseRandomRange.Y);
}