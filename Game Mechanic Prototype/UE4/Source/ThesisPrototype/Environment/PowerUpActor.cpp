// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "PowerUpActor.h"
#include "TimerManager.h"

APowerUpActor::APowerUpActor()
{
	powerUpInterval = 0.f;
	totalNumOfTicks = 0;
	bIsPowerUpActive = false;
}

void APowerUpActor::onTickPowerUp()
{
	++ticksProcessed;

	OnPowerUpTicked();

	if(ticksProcessed >= totalNumOfTicks)
	{
		setPowerUpState(false);

		OnExpired();

		GetWorldTimerManager().ClearTimer(timerHandle_PowerUpTick);
	}
}

void APowerUpActor::setPowerUpState(bool Val)
{
	bIsPowerUpActive = Val;
	onPowerUpStateChanged(Val);
}

void APowerUpActor::ActivatePowerUp(AActor* ActivatedFor)
{
	OnActivated(ActivatedFor);

	setPowerUpState(true);

	if (powerUpInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(timerHandle_PowerUpTick, this, &APowerUpActor::onTickPowerUp, powerUpInterval, true);
	}
	else
	{
		onTickPowerUp();
	}
}
