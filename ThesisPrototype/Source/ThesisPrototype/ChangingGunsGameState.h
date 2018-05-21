// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChangingGunsGameState.generated.h"

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	WaitingToStart,
	
	WaveInProgress,
	
	//no longer spawning new bots, waiting for players to kill remaining bots
	WaitingToComplete,
	
	WaveComplete,
	
	GameOver
};
/**
 * 
 */
UCLASS()
class THESISPROTOTYPE_API AChangingGunsGameState : public AGameStateBase
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_WaveState, BlueprintReadOnly, Category = "Game State")
	EWaveState WaveState;

public:
	void SetWaveState(EWaveState newState);

protected:
	UFUNCTION()
	void OnRep_WaveState(EWaveState oldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game State")
	void waveStateChanged(EWaveState newState, EWaveState oldState);
};
