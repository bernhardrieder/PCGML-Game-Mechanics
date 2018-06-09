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

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	SingleFire,
	SemiAutomatic,
	Automatic
};

UCLASS()
class THESISPROTOTYPE_API AShooterWeapon : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UCurveFloat* BaseDamageCurve;

	// bullets per minute fired
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float RateOfFire;

	// bullet spread in degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpread;

	// increase of spread per fired bullet
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	float BulletSpreadIncrease;

	// bullet spread decrease in degree, applied per second
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BulletSpreadDecrease;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category="Weapon")
	int BulletsPerMagazine;

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int AvailableMagazines;

	// amount of bullets which are fired per shot. e.g. a shotgun has 12
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	int BulletsPerShot;

	// reload time of an empty magazine in seconds
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	float ReloadTimeEmptyMagazine;

	// random recoil range in degree
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FVector RandomRecoilRange;

	// recoil decrease 
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RecoilDecrease;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	EFireMode FireMode;

	UPROPERTY(EditAnywhere, Category="Weapon")
	bool bUnlimitiedBullets;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FOnAmmoChangedEvent OnAmmoChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnReloadStateChangedEvent OnReloadStateChangedEvent;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta=(DisplayName="Bullets in magazine"))
	int m_currentBulletsInMagazine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta=(DisplayName="Available bullets left to shoot"))
	int m_availableBulletsLeft = 0;

public:
	// Sets default values for this actor's properties
	AShooterWeapon();

	virtual void StartFire();
	virtual void StopFire();
	virtual void StartMagazineReloading();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	FORCEINLINE int GetAmountOfBulletsLeftInMagazine() { return m_currentBulletsInMagazine;}

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FORCEINLINE int GetAmountOfBulletsLeftToShoot() { return m_availableBulletsLeft; }

	//call when the weapon is used 
	virtual void Equip();

	//call when the weapon is stored
	virtual void Disarm();



protected:
	virtual void BeginPlay() override;
	virtual void PlayFireEffects(const FVector& FireImpactPoint);
	virtual void PlayImpactEffects(EPhysicalSurface surfaceType, const FVector& impactPoint);
	virtual void Fire();
	virtual void reloadMagazine();
	virtual void startStockReloading();
	virtual void reloadStock();

protected:
	bool m_bIsAmmoLeftInMagazine = true;
	bool m_bIsReloading = false;

	FTimerHandle TimerHandle_AutomaticFire;
	FTimerHandle TimerHandle_ReloadMagazine;
	FTimerHandle TimerHandle_ReloadStock;
	float lastFireTime;
	//derived from rate of fire
	float timeBetweenShots;

	float m_singleBulletReloadTime;


	FVector2D m_currentRecoil = FVector2D::ZeroVector;
};
