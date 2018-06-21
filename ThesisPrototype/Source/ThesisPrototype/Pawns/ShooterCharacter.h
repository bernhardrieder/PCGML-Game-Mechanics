// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentWeaponChangedEvent, class AShooterWeapon*, NewCurrentWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponGeneratorAvailableEvent, class AWeaponGenerator*, WeaponGenerator);

class UCameraComponent;
class USpringArmComponent;
class AShooterWeapon;
class UHealthComponent;
class UDamageType;
class AController;
class AWeaponGenerator;
class AChangingGunsPlayerState;

UENUM(BlueprintType)
enum class EActorType : uint8
{
	Player,
	Bot,
	BotBoss
};

UCLASS()
class THESISPROTOTYPE_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* cameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* springArmComp;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float zoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100.0))
	float zoomInterpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TArray<TSubclassOf<AShooterWeapon>> starterWeaponClasses;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<AWeaponGenerator> bp_weaponGenerator;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Player")
	UHealthComponent* healthComp;

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bDied;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Weapon", meta = (DisplayName = "Equipped Weapon"))
	AShooterWeapon* equippedWeapon;

public:
	AShooterCharacter();

	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartFire();
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopFire();
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsMoving() const;
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsCrouching() const;
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsAiming() const { return bWantsToZoom; }

	FORCEINLINE AShooterWeapon* GetEquippedWeapon() const { return equippedWeapon; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void moveForward(float Value);
	void moveRight(float Value);
	void beginCrouch();
	void endCrouch();
	void beginZoom();
	void endZoom();
	void beginRun();
	void endRun();
	void reloadWeapon();
	void addWeapon(AShooterWeapon* Weapon);
	void equipWeapon(AShooterWeapon* Weapon);
	void disarmWeapon(AShooterWeapon* Weapon);
	//switches to next weapon if val is positive otherwise to the previous weapon
	void switchWeapon(float Value);
	void switchToLastEquipedWeapon();
	void removeWeapon(AShooterWeapon* Weapon);
	void dismantleEquippedWeaponAndGenerateNew();

	//this is the callback for the weapon generator
	UFUNCTION()
	void onNewWeaponGenerated(AShooterWeapon* Weapon);

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	FName getSocketNameFor(const AShooterWeapon* Weapon) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnCurrentWeaponChangedEvent OnCurrentWeaponChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Player|Weapon Generator")
	FOnWeaponGeneratorAvailableEvent OnWeaponGeneratorAvailableEvent;

protected:
	bool bWantsToZoom = false;
	float defaultFOV = 90;
	float maxWalkSpeedDefault = 0;
	float maxWalkSpeedCrouchedDefault = 0;

	AShooterWeapon* lastEquippedWeapon;
	TArray<AShooterWeapon*> availableWeapons;
	AWeaponGenerator* weaponGenerator;
};
