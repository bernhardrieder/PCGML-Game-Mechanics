// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShooterTrackerBot.generated.h"

class UHealthComponent;
class USphereComponent;
class USoundCue;
class UAudioComponent;

UCLASS()
class THESISPROTOTYPE_API AShooterTrackerBot : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* meshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UHealthComponent* healthComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USphereComponent* sphereComp;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* explosionEffect;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UAudioComponent* movementAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float movementForce;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float requiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float explosionDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	TSubclassOf<UDamageType> damageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float damageRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float selfDamageInterval;

	UPROPERTY()
	bool bExploded = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* selfDestructSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* explosionSound;

	//power level increases with same bots nearby
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	int32 maxPowerLevel;

public:
	// Sets default values for this pawn's properties
	AShooterTrackerBot();
	void Tick(float DeltaTime) override;
	void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	void BeginPlay() override;

	FVector getNextPathPoint();
	void selfDestruct();
	void damageSelf();
	void onCheckNearbyBots();
	void refreshPath();

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser);

protected:
	FVector nextPathPoint;
	UMaterialInstanceDynamic* materialInstance = nullptr;
	FTimerHandle timerHandle_selfDamage;
	FTimerHandle timerHandle_refreshPath;
	bool bStartedSelfDestruction = false;

	//current power level of the bot based on nearby located bots -> this boosts the explosion damage
	int32 currentPowerLevel = 0;
};
