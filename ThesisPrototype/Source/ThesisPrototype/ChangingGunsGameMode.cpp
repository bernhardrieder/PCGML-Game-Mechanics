// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/HealthComponent.h"

AChangingGunsGameMode::AChangingGunsGameMode() : Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;

	TimeBetweenWaves = 2.f;
}

void AChangingGunsGameMode::StartPlay()
{
	Super::StartPlay();

	prepareForNextWave();
}

void AChangingGunsGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	checkWaveState();
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
}

void AChangingGunsGameMode::prepareForNextWave()
{
	GetWorldTimerManager().SetTimer(timerHandle_NextWaveStart, this, &AChangingGunsGameMode::startWave, TimeBetweenWaves, false);
}

void AChangingGunsGameMode::checkWaveState()
{
	const bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(timerHandle_NextWaveStart);

	if(NumOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bIsAnyBotAlive = false;
	for(FConstPawnIterator it = GetWorld()->GetPawnIterator(); it; ++it)
	{
		APawn* testPawn = it->Get();
		if(!testPawn || testPawn->IsPlayerControlled())
		{
			continue;
		}
		UHealthComponent* healthComp = Cast<UHealthComponent>(testPawn->GetComponentByClass(UHealthComponent::StaticClass()));
		if(healthComp && healthComp->GetHealth() > 0.f)
		{
			bIsAnyBotAlive = true;
			break;
		}
	}

	if(!bIsAnyBotAlive)
	{
		prepareForNextWave();
	}
}
