// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameState.h"
#include "UnrealNetwork.h"

void AChangingGunsGameState::SetWaveState(EWaveState newState)
{
	const EWaveState oldState = WaveState;
	WaveState = newState;
	waveStateChanged(WaveState, oldState);
}
