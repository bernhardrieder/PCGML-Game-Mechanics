// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/HealthComponent.h"
#include "ChangingGunsGameState.h"
#include "ChangingGunsPlayerState.h"

AChangingGunsGameMode::AChangingGunsGameMode() : Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;

	GameStateClass = AChangingGunsGameState::StaticClass();
	PlayerStateClass = AChangingGunsPlayerState::StaticClass();

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
	checkAnyPlayerAlive();
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

	setWaveState(EWaveState::WaveInProgress);
}

void AChangingGunsGameMode::endWave()
{
	GetWorldTimerManager().ClearTimer(timerHandle_BotSpawner);

	setWaveState(EWaveState::WaitingToComplete);
}

void AChangingGunsGameMode::prepareForNextWave()
{
	GetWorldTimerManager().SetTimer(timerHandle_NextWaveStart, this, &AChangingGunsGameMode::startWave, TimeBetweenWaves, false);

	setWaveState(EWaveState::WaitingToStart);

	restartDeadPlayers();
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

	//TODO: change this to check if just 1 player is alive
	//no player alive
	gameOver();
}

void AChangingGunsGameMode::gameOver()
{
	endWave();

	setWaveState(EWaveState::GameOver);
	UE_LOG(LogTemp, Log, TEXT("Game over! All players are dead!"));
}

void AChangingGunsGameMode::setWaveState(EWaveState newState)
{
	if(AChangingGunsGameState* gameState = GetGameState<AChangingGunsGameState>())
	{
		gameState->SetWaveState(newState);
	}
}

void AChangingGunsGameMode::restartDeadPlayers()
{
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		APlayerController* pc = it->Get();
		if (pc && !pc->GetPawn())
		{
			RestartPlayer(pc);
		}
	}
}
