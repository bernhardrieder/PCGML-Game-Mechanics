// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "PowerUpActor.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

APowerUpActor::APowerUpActor()
{
	PowerUpInterval = 0.f;
	TotalNumOfTicks = 0;
	bIsPowerUpActive = false;
}

void APowerUpActor::onTickPowerUp()
{
	++ticksProcessed;

	OnPowerUpTicked();

	if(ticksProcessed >= TotalNumOfTicks)
	{
		setPowerUpState(false);

		OnExpired();

		GetWorldTimerManager().ClearTimer(timerHandle_PowerUpTick);
	}
}

void APowerUpActor::setPowerUpState(bool val)
{
	bIsPowerUpActive = val;
	OnPowerUpStateChanged(val);
}

void APowerUpActor::ActivatePowerUp(AActor* ActivatedFor)
{
	OnActivated(ActivatedFor);

	setPowerUpState(true);

	if (PowerUpInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(timerHandle_PowerUpTick, this, &APowerUpActor::onTickPowerUp, PowerUpInterval, true);
	}
	else
	{
		onTickPowerUp();
	}
}
