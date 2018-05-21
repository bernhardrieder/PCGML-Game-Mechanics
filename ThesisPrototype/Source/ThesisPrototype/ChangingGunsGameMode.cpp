// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameMode.h"
#include "TimerManager.h"

AChangingGunsGameMode::AChangingGunsGameMode() : Super()
{
	TimeBetweenWaves = 2.f;
}

void AChangingGunsGameMode::StartPlay()
{
	Super::StartPlay();

	prepareForNextWave();
}

void AChangingGunsGameMode::spawnBotTimerElapsed()
{
	spawnNewBot();

	--NumOfBotsToSpawn;
	if(NumOfBotsToSpawn <= 0)
	{
		endWave();
	}
}

void AChangingGunsGameMode::startWave()
{
	++WaveCount;
	NumOfBotsToSpawn = 2 * WaveCount;
	GetWorldTimerManager().SetTimer(timerHandle_BotSpawner, this, &AChangingGunsGameMode::spawnBotTimerElapsed, 1.f, true, 0.f);

}

void AChangingGunsGameMode::endWave()
{
	GetWorldTimerManager().ClearTimer(timerHandle_BotSpawner);
	
	prepareForNextWave();
}

void AChangingGunsGameMode::prepareForNextWave()
{
	FTimerHandle timerHandle_NextWaveStart;
	GetWorldTimerManager().SetTimer(timerHandle_NextWaveStart, this, &AChangingGunsGameMode::startWave, TimeBetweenWaves, false);
}
