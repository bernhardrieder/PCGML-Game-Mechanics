// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsGameState.h"
#include "UnrealNetwork.h"

void AChangingGunsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChangingGunsGameState, WaveState);
}

void AChangingGunsGameState::SetWaveState(EWaveState newState)
{
	if(HasAuthority())
	{
		const EWaveState oldState = WaveState;
		WaveState = newState;
		OnRep_WaveState(oldState); // to call this also on the server!
	}
}

void AChangingGunsGameState::OnRep_WaveState(EWaveState oldState)
{
	waveStateChanged(WaveState, oldState);
}
