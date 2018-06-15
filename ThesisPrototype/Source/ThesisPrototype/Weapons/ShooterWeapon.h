// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedEvent, int, overallAvailableBulletsLeftToShoot, int, amountOfBulletsLeftInMagazine);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnReloadStateChangedEvent, bool, isReloading, float, timeToFinish, int, amountOfBulletsInMagazine);


class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;
class UCurveFloat;
struct FRuntimeFloatCurve;
class AShooterCharacter;
class UAudioComponent;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Pistol,
	Shotgun,
	SubMachineGun,
	Rifle,
	SniperRifle,
	HeavyMachineGun
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	SingleFire,
	SemiAutomatic,
	Automatic
};

USTRUCT(BlueprintType)
struct FWeaponStatistics
{
	GENERATED_BODY()

	FWeaponStatistics()
	{
		Kills = 0;
		SecondsUsed = 0;
	}

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Statistics")
	int32 Kills;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Statistics")
	float SecondsUsed;
};

USTRUCT(BlueprintType)
struct FOwnerBasedModifier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OwnerBasedModifier")
	float Moving;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OwnerBasedModifier")
	float Crouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OwnerBasedModifier")
	float Aiming;

	float GetCurrentModifier(AShooterCharacter* character);
};

UCLASS()
class THESISPROTOTYPE_API AShooterWeapon : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* meshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName muzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName tracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* muzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* defaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* fleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* tracerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> fireCamShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	TSubclassOf<UDamageType> damageType;

	//x = damage, y = distance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	FVector2D maxDamageWithDistance;

	//x = damage, y = distance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	FVector2D minDamageWithDistance;

	// bullets per minute fired
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 rateOfFire;

	//this value hast absolutely no effect. this is just needed for feeding into the generator!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 muzzleVelocity;

	// increase of spread per fired bullet
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Bullet Spread")
	float bulletSpreadIncrease;

	// bullet spread decrease in degree, applied per second
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Bullet Spread")
	float bulletSpreadDecrease;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo")
	int32 bulletsPerMagazine;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 availableMagazines;

	// amount of bullets which are fired in one shot. e.g. a shotgun has 12
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo")
	int32 bulletsInOneShot;

	// reload time of an empty magazine in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo")
	float reloadTimeEmptyMagazine;

	// random recoil in degree (x = horizontal, y = vertical)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
	FVector2D recoilIncreasePerShot;

	// recoil decrease
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
	float recoilDecrease;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EFireMode fireMode;

	UPROPERTY(EditAnywhere, Category="Weapon|Ammo")
	bool bUnlimitiedBullets;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo", meta=(DisplayName="Bullets in Magazine"))
	int32 currentBulletsInMagazine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo", meta=(DisplayName="Available Bullets Left to Shoot"))
	int32 availableBulletsLeft = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponType type;

	//character walking speed modifier in percent/100, e.g., 0.8
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Walking Speed Modifier", ClampMin = 0.0, ClampMax = 1.0))
	float walkinSpeedModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Owner-based Spread Modifier"))
	FOwnerBasedModifier spreadModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Owner-based Recoil Modifier"))
	FOwnerBasedModifier recoilModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|SFX")
	USoundCue* fireSound;

public:
	// Sets default values for this actor's properties
	AShooterWeapon();

	virtual void StartFire();
	virtual void StopFire();
	virtual void StartMagazineReloading();

	//call when the weapon is currently used as main weapon
	virtual void Equip(AShooterCharacter* EuqippedBy);

	//call when the weapon is stored inventory
	virtual void Disarm();

	UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
	virtual void RefillAmmunition(int AmountOfBullets);

	FORCEINLINE float GetWalkinSpeedModifier() const { return walkinSpeedModifier; }
	FORCEINLINE EWeaponType GetType() const { return type; }
	FORCEINLINE EFireMode GetFireMode() const { return fireMode; }
	FORCEINLINE FVector2D GetMaxDamageWithDistance() const { return maxDamageWithDistance;}
	FORCEINLINE FVector2D GetMinDamageWithDistance() const { return minDamageWithDistance; }
	FORCEINLINE int32 GetRateOfFire() const { return rateOfFire; }
	FORCEINLINE int32 GetMuzzleVelocity() const { return muzzleVelocity; }
	FORCEINLINE float GetBulletSpreadIncrease() const { return bulletSpreadIncrease; }
	FORCEINLINE float GetBulletSpreadDecrease() const { return bulletSpreadDecrease; }
	FORCEINLINE FVector2D GetRecoilIncreasePerShot() const { return recoilIncreasePerShot; }
	FORCEINLINE float GetRecoilDecrease() const { return recoilDecrease; }
	FORCEINLINE int32 GetBulletsPerMagazine() const { return bulletsPerMagazine; }
	FORCEINLINE int32 GetBulletsInOneShot() const { return bulletsInOneShot; }
	FORCEINLINE float GetReloadTimeEmptyMagazine() const { return reloadTimeEmptyMagazine; }
	FORCEINLINE FWeaponStatistics GetWeaponStatistics() const { return statistics; }

	FORCEINLINE void SetType(EWeaponType Type) { type = Type; }
	FORCEINLINE void SetFireMode(EFireMode Firemode) {  fireMode = Firemode; }
	void SetMaxDamageWithDistance(const FVector2D& MaxDamageWithDistance);
	void SetMinDamageWithDistance(const FVector2D& MinDamageWithDistance);
	void SetRateOfFire(int32 RateOfFire);
	FORCEINLINE void SetMuzzleVelocity(int32 MuzzleVelocity) { muzzleVelocity = MuzzleVelocity; }
	FORCEINLINE void SetBulletSpreadIncrease(float Increase) {  bulletSpreadIncrease = Increase; }
	FORCEINLINE void SetBulletSpreadDecrease(float Decrease) {  bulletSpreadDecrease = Decrease; }
	FORCEINLINE void SetRecoilIncreasePerShot(const FVector2D& Increase) {  recoilIncreasePerShot = Increase; }
	FORCEINLINE void SetRecoilDecrease(float Decrease) {  recoilDecrease = Decrease; }
	void SetBulletsPerMagazine(int32 Bullets);
	FORCEINLINE void SetBulletsInOneShot(int32 Bullets) {  bulletsInOneShot = Bullets; }
	FORCEINLINE void SetReloadTimeEmptyMagazine(float Time);

protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	
	void playFireEffects(const FVector& FireImpactPoint);
	void playImpactEffects(EPhysicalSurface SurfaceType, const FVector& ImpactPoint);
	void fire();
	void reloadMagazine();
	void startStockReloading();
	void reloadStock();
	//calculates bullet spread dispersion (x = horizontal, y = vertical)
	FVector2D calculateBulletSpreadDispersion(float RandomPower, float CurrentSpread);
	void decreaseBulletSpread();
	void applyRecoil();
	void compensateRecoil(float DeltaTime);
	float calculateRecoilCompensationDelta(float DeltaTime, float CurrentRecoil);
	float getDamageMultiplierFor(EPhysicalSurface SurfaceType);
	void buildDamageCurve();
	void updateSingleBulletReloadTime();

public:
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChangedEvent OnAmmoChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadStateChangedEvent OnReloadStateChangedEvent;

protected:
	//derived
	float timeBetweenShots = 0;
	float singleBulletReloadTime = 0;
	FRuntimeFloatCurve damageCurve;

	AShooterCharacter* owningCharacter;
	bool bIsAmmoLeftInMagazine = true;
	bool bIsReloading = false;
	FVector2D currentRecoil;
	float currentBulletSpread = 0;
	float lastFireTime = 0;
	float timeEquipped = 0;

	FTimerHandle timerHandle_AutomaticFire;
	FTimerHandle timerHandle_ReloadMagazine;
	FTimerHandle timerHandle_ReloadStock;
	FTimerHandle timerHandle_SpreadDecrease;

	FWeaponStatistics statistics;
};
