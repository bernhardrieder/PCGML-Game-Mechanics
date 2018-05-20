// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "PowerUpActor.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

void APowerUpActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APowerUpActor, bIsPowerUpActive);
}

APowerUpActor::APowerUpActor()
{
	PowerUpInterval = 0.f;
	TotalNumOfTicks = 0;
	bIsPowerUpActive = false;

	SetReplicates(true);
}

void APowerUpActor::onTickPowerUp()
{
	++ticksProcessed;

	OnPowerUpTicked();

	if(ticksProcessed >= TotalNumOfTicks)
	{
		bIsPowerUpActive = false;
		OnRep_PowerUpActive();

		OnExpired();

		GetWorldTimerManager().ClearTimer(timerHandle_PowerUpTick);
	}
}

void APowerUpActor::OnRep_PowerUpActive()
{
	OnPowerUpStateChanged(bIsPowerUpActive);
}

void APowerUpActor::ActivatePowerUp(AActor* ActivatedFor)
{
	OnActivated(ActivatedFor);

	bIsPowerUpActive = true;
	OnRep_PowerUpActive(); //server does not call this function automatically

	if (PowerUpInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(timerHandle_PowerUpTick, this, &APowerUpActor::onTickPowerUp, PowerUpInterval, true);
	}
	else
	{
		onTickPowerUp();
	}
}
