// Fill out your copyright notice in the Description page of Project Settings.

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
		if(GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, ECC_Visibility, queryParams))
		{
			//is blocking hit! -> process damage
			AActor* hitActor = hitResult.GetActor();
			UGameplayStatics::ApplyPointDamage(hitActor, 20.f, shootDirection, hitResult, owner->GetInstigatorController(), this, DamageType);


			UParticleSystem* selectedEffect = nullptr;
			EPhysicalSurface surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());
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
