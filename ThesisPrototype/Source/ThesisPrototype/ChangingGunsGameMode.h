// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChangingGunsGameMode.generated.h"

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

public:
	AChangingGunsGameMode();
	virtual void StartPlay() override;

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

protected:
	FTimerHandle timerHandle_BotSpawner;
	int32 NumOfBotsToSpawn;
	int32 WaveCount;
};
