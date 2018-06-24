// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/HealthComponent.h"
#include "ChangingGunsGameState.h"
#include "ChangingGunsPlayerState.h"
#include "Pawns/ShooterCharacter.h"
#include "ChangingGuns.h"

AChangingGunsGameMode::AChangingGunsGameMode() : Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;

	GameStateClass = AChangingGunsGameState::StaticClass();
	PlayerStateClass = AChangingGunsPlayerState::StaticClass();

	timeBetweenWaves = 2.f;
	botsPerWaveMultiplier = 2;
}

void AChangingGunsGameMode::StartPlay()
{
	Super::StartPlay();
	gameState = GetGameState<AChangingGunsGameState>();
	prepareForNextWave();
}

void AChangingGunsGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	checkWaveState();
	checkAnyPlayerAlive();
}

void AChangingGunsGameMode::spawnBotTimerElapsed()
{
	spawnNewBot();

	--numOfBotsToSpawn;
	if(numOfBotsToSpawn <= 0)
	{
		endWave();
	}
}

void AChangingGunsGameMode::startWave()
{
	++waveCount;
	numOfBotsToSpawn = botsPerWaveMultiplier * waveCount;
	GetWorldTimerManager().SetTimer(timerHandle_BotSpawner, this, &AChangingGunsGameMode::spawnBotTimerElapsed, 1.f, true, 0.f);

	setWaveState(EWaveState::WaveInProgress);
}

void AChangingGunsGameMode::endWave()
{
	GetWorldTimerManager().ClearTimer(timerHandle_BotSpawner);

	setWaveState(EWaveState::WaitingToComplete);
}

void AChangingGunsGameMode::prepareForNextWave()
{
	GetWorldTimerManager().SetTimer(timerHandle_NextWaveStart, this, &AChangingGunsGameMode::startWave, timeBetweenWaves, false);

	setWaveState(EWaveState::WaitingToStart);
}

void AChangingGunsGameMode::checkWaveState()
{
	if(gameState->GetWaveState() == EWaveState::BossFight)
	{
		GetWorldTimerManager().ClearTimer(timerHandle_BotSpawner);
		GetWorldTimerManager().ClearTimer(timerHandle_NextWaveStart);
		return;
	}

	const bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(timerHandle_NextWaveStart);

	if(numOfBotsToSpawn > 0 || bIsPreparingForWave)
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
		if(healthComp && healthComp->GetTeamNumber() == TEAMNUMBER_BOT && healthComp->IsHandlingDamage() && healthComp->GetHealth() > 0.f)
		{
			bIsAnyBotAlive = true;
			break;
		}
	}

	if(!bIsAnyBotAlive)
	{
		setWaveState(EWaveState::WaveComplete);
		prepareForNextWave();
	}
}

void AChangingGunsGameMode::checkAnyPlayerAlive()
{
	for(FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		APlayerController* pc = it->Get();
		if(pc && pc->GetPawn())
		{
			APawn* pawn = pc->GetPawn();
			UHealthComponent* healthComp = Cast<UHealthComponent>(pawn->GetComponentByClass(UHealthComponent::StaticClass()));
			if(ensure(healthComp) && healthComp->GetHealth() > 0.f)
			{
				return;
			}
		}
	}

	gameOver();
}

void AChangingGunsGameMode::gameOver()
{
	endWave();
	setWaveState(EWaveState::GameOver);
}

void AChangingGunsGameMode::setWaveState(EWaveState NewState)
{
	gameState->SetWaveState(NewState);
}