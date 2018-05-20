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
	//time between power up ticks
	UPROPERTY(EditDefaultsOnly, Category = "Power Ups")
	float PowerUpInterval;

	//total times we apply the power up effect
	UPROPERTY(EditDefaultsOnly, Category = "Power Ups")
	int32 TotalNumOfTicks;
	
public:	
	// Sets default values for this actor's properties
	APowerUpActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void onTickPowerUp();

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
