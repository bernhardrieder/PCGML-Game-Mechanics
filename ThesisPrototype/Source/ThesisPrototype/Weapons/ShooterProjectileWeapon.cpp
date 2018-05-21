// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

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
