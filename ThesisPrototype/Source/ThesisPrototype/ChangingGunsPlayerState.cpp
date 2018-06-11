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

void AChangingGunsPlayerState::AddKill(AShooterWeapon* weapon)
{
	FWeaponStatistics& stats = getWeaponStats(weapon);
	++stats.Kills;
	if(DebugPlayerStateOutput)
	{
		UE_LOG(LogTemp, Log, TEXT("Kills with %s: %i"), *stats.Weapon->GetName(), stats.Kills);
	}
}

void AChangingGunsPlayerState::AddUsageTime(AShooterWeapon* weapon, float secondsUsed)
{
	FWeaponStatistics& stats = getWeaponStats(weapon);
	stats.SecondsUsed += secondsUsed;
	if (DebugPlayerStateOutput)
	{
		UE_LOG(LogTemp, Log, TEXT("Seconds used with %s: %i"), *stats.Weapon->GetName(), stats.SecondsUsed);
	}
}

FWeaponStatistics AChangingGunsPlayerState::GetWeaponStats(AShooterWeapon* weapon)
{
	return getWeaponStats(weapon);
}

void AChangingGunsPlayerState::RemoveWeaponFromStatistics(AShooterWeapon* weapon)
{
	int32 idx = findWeaponStatsIndexInArray(weapon);
	if(idx != -1)
	{
		m_weaponStats.RemoveAt(idx);
		if (DebugPlayerStateOutput)
		{
			UE_LOG(LogTemp, Log, TEXT("Removed %s from statistics"), *weapon->GetName());
		}
	}
}

int32 AChangingGunsPlayerState::findWeaponStatsIndexInArray(AShooterWeapon* weapon)
{
	int32 idx = -1;

	for (int32 i = 0; i < m_weaponStats.Num(); ++i)
	{
		if (m_weaponStats[i].Weapon == weapon)
		{
			idx = i;
			break;
		}
	}
	return idx;
}

FWeaponStatistics& AChangingGunsPlayerState::getWeaponStats(AShooterWeapon* weapon)
{
	int32 idx = findWeaponStatsIndexInArray(weapon);
	if (idx == -1)
	{
		FWeaponStatistics stats;
		stats.Weapon = weapon;
		idx = m_weaponStats.Add(stats);
	}
	return m_weaponStats[idx];
}
