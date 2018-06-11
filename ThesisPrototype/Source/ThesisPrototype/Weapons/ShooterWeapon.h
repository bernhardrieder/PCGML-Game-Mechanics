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
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	TSubclassOf<UDamageType> DamageType;

	//x = damage, y = distance
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Damage")
	FRuntimeFloatCurve DamageCurve;

	// bullets per minute fired
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float RateOfFire;

	// increase of spread per fired bullet
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Bullet Spread")
	float BulletSpreadIncrease;

	// bullet spread decrease in degree, applied per second
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Bullet Spread")
	float BulletSpreadDecrease;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category="Weapon|Ammo")
	int BulletsPerMagazine;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int AvailableMagazines;

	// amount of bullets which are fired in one shot. e.g. a shotgun has 12
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Ammo")
	int BulletsInOneShot;

	// reload time of an empty magazine in seconds
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Ammo")
	float ReloadTimeEmptyMagazine;

	// random recoil in degree (x = horizontal, y = vertical)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
	FVector2D RecoilIncreasePerShot;

	// recoil decrease 
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
	float RecoilDecrease;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	EFireMode FireMode;

	UPROPERTY(EditAnywhere, Category="Weapon|Ammo")
	bool bUnlimitiedBullets;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnAmmoChangedEvent OnAmmoChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadStateChangedEvent OnReloadStateChangedEvent;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo", meta=(DisplayName="Bullets in Magazine"))
	int m_currentBulletsInMagazine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo", meta=(DisplayName="Available Bullets Left to Shoot"))
	int m_availableBulletsLeft = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	EWeaponType Type;

	//character walking speed modifier in percent/100, e.g., 0.8
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Walking Speed Modifier", ClampMin = 0.0, ClampMax = 1.0))
	float m_walkinSpeedModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Owner-based Spread Modifier"))
	FOwnerBasedModifier m_spreadModifier;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Modifier", meta = (DisplayName = "Owner-based Recoil Modifier"))
	FOwnerBasedModifier m_recoilModifier;

public:
	// Sets default values for this actor's properties
	AShooterWeapon();

	virtual void StartFire();
	virtual void StopFire();
	virtual void StartMagazineReloading();

	//call when the weapon is currently used as main weapon
	virtual void Equip(AShooterCharacter* euqippedBy);
	//call when the weapon is stored inventory
	virtual void Disarm();

	UFUNCTION(BlueprintCallable, Category="Weapon|Ammo")
	virtual void RefillAmmunition(int amountOfBullets);

	FORCEINLINE float GetWalkinSpeedModifier() const { return m_walkinSpeedModifier; }
	FORCEINLINE EWeaponType GetType() const { return Type; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void PlayFireEffects(const FVector& FireImpactPoint);
	virtual void PlayImpactEffects(EPhysicalSurface surfaceType, const FVector& impactPoint);
	virtual void Fire();
	virtual void reloadMagazine();
	virtual void startStockReloading();
	virtual void reloadStock();
	//calculates bullet spread dispersion (x = horizontal, y = vertical)
	FVector2D calculateBulletSpreadDispersion(float randomPower, float currentSpread);
	void decreaseBulletSpread();
	void applyRecoil();
	void compensateRecoil(float deltaTime);
	float calculateRecoilCompensationDelta(float deltaTime, float currentRecoil);

	float getDamageMultiplierFor(EPhysicalSurface surfaceType);

protected:
	AShooterCharacter* m_owningCharacter;
	
	bool m_bIsAmmoLeftInMagazine = true;
	bool m_bIsReloading = false;

	FTimerHandle TimerHandle_AutomaticFire;
	FTimerHandle TimerHandle_ReloadMagazine;
	FTimerHandle TimerHandle_ReloadStock;
	FTimerHandle TimerHandle_SpreadDecrease;


	float m_currentBulletSpread = 0;

	float lastFireTime;
	//derived from rate of fire
	float timeBetweenShots;

	float m_singleBulletReloadTime;

	FVector2D m_currentRecoil;


};
