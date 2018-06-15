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

	BossFight,
	
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
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	EWaveState waveState;

public:
	UFUNCTION(BlueprintCallable, Category = "Game State")
	void SetWaveState(EWaveState NewState);

	FORCEINLINE EWaveState GetWaveState() const { return waveState; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Game State")
	void waveStateChanged(EWaveState NewState, EWaveState OldState);
};
