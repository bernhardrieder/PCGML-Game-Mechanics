// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ShooterWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Camera/CameraShake.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ChangingGuns.h"
#include "TimerManager.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing (
	TEXT("Game.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat
);

// Sets default values
AShooterWeapon::AShooterWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";
	BaseDamage = 20.f;
	RateOfFire = 600; //bullets per minute
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	timeBetweenShots = 60 / RateOfFire;
}

void AShooterWeapon::StartFire()
{
	float firstDelay = FMath::Max(lastFireTime + timeBetweenShots - GetWorld()->TimeSeconds, 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AShooterWeapon::Fire, timeBetweenShots, true, firstDelay);
}

void AShooterWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void AShooterWeapon::Fire()
{
	//trace the world, from pawn eyes to crosshair location

	if(AActor* owner = GetOwner())
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		owner->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		FVector shootDirection = eyeRotator.Vector();
		FVector traceEnd = eyeLocation + shootDirection * 10000;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(owner);
		queryParams.AddIgnoredActor(this);
		queryParams.bTraceComplex = true; //gives us the exact result because traces every triangle instead of a simple collider
		queryParams.bReturnPhysicalMaterial = true;

		FVector tracerEndPoint = traceEnd;
		FHitResult hitResult;
		if(GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams))
		{
			//is blocking hit! -> process damage
			float actualDamage = BaseDamage;

			AActor* hitActor = hitResult.GetActor();

			EPhysicalSurface surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

			if(surfaceType == SURFACE_FLESHVULNERABLE)
			{
				actualDamage *= 4.f;
			}
			UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shootDirection, hitResult, owner->GetInstigatorController(), this, DamageType);

			UParticleSystem* selectedEffect = nullptr;
			switch(surfaceType)
			{
				case SURFACE_FLESHDEFAULT:
				case SURFACE_FLESHVULNERABLE:
					selectedEffect = FleshImpactEffect;
					break;
				default:
					selectedEffect = DefaultImpactEffect;
					break;
			}

			if (selectedEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), selectedEffect, hitResult.ImpactPoint, hitResult.ImpactNormal.Rotation());
			}

			tracerEndPoint = hitResult.ImpactPoint;
		}

		if(DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);
		}

		PlayFireEffects(tracerEndPoint);

		lastFireTime = GetWorld()->TimeSeconds;
	}

}

void AShooterWeapon::PlayFireEffects(const FVector& FireImpactPoint)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* particleSystem = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, muzzleLocation);
		particleSystem->SetVectorParameter(TracerTargetName, FireImpactPoint);
	}

	if(APawn* owner = Cast<APawn>(GetOwner()))
	{
		if(APlayerController* pc = Cast<APlayerController>(owner->GetController()))
		{
			pc->ClientPlayCameraShake(FireCamShake);
		}
	}
}
