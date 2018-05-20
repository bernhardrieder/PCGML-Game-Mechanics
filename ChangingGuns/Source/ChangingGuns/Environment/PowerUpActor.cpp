// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "PowerUpActor.h"
#include "TimerManager.h"

// Sets default values
APowerUpActor::APowerUpActor()
{
	PowerUpInterval = 0.f;
	TotalNumOfTicks = 0;
}

// Called when the game starts or when spawned
void APowerUpActor::BeginPlay()
{
	Super::BeginPlay();


}

void APowerUpActor::onTickPowerUp()
{
	++ticksProcessed;

	OnPowerUpTicked();

	if(ticksProcessed >= TotalNumOfTicks)
	{
		OnExpired();

		GetWorldTimerManager().ClearTimer(timerHandle_PowerUpTick);
	}
}

void APowerUpActor::ActivatePowerUp()
{
	if (PowerUpInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(timerHandle_PowerUpTick, this, &APowerUpActor::onTickPowerUp, PowerUpInterval, true, 0.f);
	}
	else
	{
		onTickPowerUp();
	}
}
