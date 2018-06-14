// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ChangingGunsPlayerState.h"
#include "Weapons/ShooterWeapon.h"

static int32 DebugPlayerStateOutput = 1;
FAutoConsoleVariableRef CVARDebugPlayerStateOutput(
	TEXT("Game.DebugPlayerState"),
	DebugPlayerStateOutput,
	TEXT("Print Debug Output for AChangingGunsPlayerState"),
	ECVF_Cheat
);

