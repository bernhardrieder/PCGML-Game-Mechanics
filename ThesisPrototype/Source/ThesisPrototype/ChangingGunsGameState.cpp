// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameState.h"
#include "UnrealNetwork.h"

void AChangingGunsGameState::SetWaveState(EWaveState NewState)
{
	const EWaveState oldState = waveState;
	waveState = NewState;
	waveStateChanged(waveState, oldState);
}
