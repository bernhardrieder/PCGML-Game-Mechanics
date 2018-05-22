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
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComp;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UAudioComponent* MovementAudioComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float RequiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float DamageRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float SelfDamageInterval;

	UPROPERTY()
	bool bExploded = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* ExplosionSound;

	//power level increases with same bots nearby
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	int32 MaxPowerLevel;

public:
	// Sets default values for this pawn's properties
	AShooterTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector GetNextPathPoint();

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser);

	void selfDestruct();
	void damageSelf();
	void onCheckNearbyBots();
	void refreshPath();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	FVector nextPathPoint;
	UMaterialInstanceDynamic* materialInstance = nullptr;
	FTimerHandle timerHandle_SelfDamage;
	FTimerHandle timerHandle_RefreshPath;
	bool bStartedSelfDestruction = false;

	//current power level of the bot based on nearby located bots -> this boosts the explosion damage
	int32 currentPowerLevel = 0;
};
