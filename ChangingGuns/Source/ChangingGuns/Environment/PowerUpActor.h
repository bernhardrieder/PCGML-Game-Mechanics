// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerUpActor.generated.h"

UCLASS()
class CHANGINGGUNS_API APowerUpActor : public AActor
{
	GENERATED_BODY()

protected:
	//time in seconds between power up ticks
	UPROPERTY(EditDefaultsOnly, Category = "Power Ups")
	float PowerUpInterval;

	//total times the power up effect ticks and applies any effect
	UPROPERTY(EditDefaultsOnly, Category = "Power Ups")
	int32 TotalNumOfTicks;

	UPROPERTY(ReplicatedUsing=OnRep_PowerUpActive)
	bool bIsPowerUpActive;
public:	
	// Sets default values for this actor's properties
	APowerUpActor();

protected:
	void onTickPowerUp();

	UFUNCTION()
	void OnRep_PowerUpActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Power Ups")
	void OnPowerUpStateChanged(bool bNewIsActive);

public:
	void ActivatePowerUp();

	UFUNCTION(BlueprintImplementableEvent, Category = "Power Ups")
	void OnActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Power Ups")
	void OnPowerUpTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Power Ups")
	void OnExpired();

protected:
	FTimerHandle timerHandle_PowerUpTick;
	int32 ticksProcessed = 0;
};
