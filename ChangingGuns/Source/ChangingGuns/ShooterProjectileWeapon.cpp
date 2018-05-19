// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterProjectileWeapon.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

void AShooterProjectileWeapon::Fire()
{
	if (AActor* owner = GetOwner())
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		owner->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		if(ProjectileClass)
		{
			FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
			FRotator muzzleRotator = MeshComp->GetSocketRotation(MuzzleSocketName);

			FActorSpawnParameters spawnParams;
			spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<AActor>(ProjectileClass, muzzleLocation, eyeRotator, spawnParams);
		}
	
	}
}
