// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChangingGunsGameMode.generated.h"

class AChangingGunsGameState;
enum class EWaveState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilledEvent, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);

/**
 *
 */
UCLASS()
class THESISPROTOTYPE_API AChangingGunsGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Game Mode")
	float TimeBetweenWaves;

	// e.g. 2 -> bots = multiplier * wavecount
	UPROPERTY(EditDefaultsOnly, Category = "Game Mode")
	int32 BotsPerWaveMultiplier;

public:
	AChangingGunsGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	//hook for BP to spawn a single bot
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode")
	void spawnNewBot();

	void spawnBotTimerElapsed();

	//start spawning bots
	void startWave();

	//Stop spawning bots
	void endWave();
	
	//set timer for next wave start
	void prepareForNextWave();

	void checkWaveState();

	void checkAnyPlayerAlive();

	void gameOver();

	void setWaveState(EWaveState newState);

	UFUNCTION()
	void onActorKilled(AActor* victimActor, AActor* killerActor, AController* killerController);

public:
	UPROPERTY(BlueprintAssignable, Category = "Game Mode")
	FOnActorKilledEvent OnActorKilledEvent;

protected:
	AChangingGunsGameState* m_gameState;
	FTimerHandle timerHandle_BotSpawner;
	int32 NumOfBotsToSpawn;
	int32 WaveCount;
	FTimerHandle timerHandle_NextWaveStart;

};
