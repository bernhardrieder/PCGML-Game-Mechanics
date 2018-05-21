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
#include "UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing (
	TEXT("Game.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat
);

void AShooterWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AShooterWeapon, HitScanTrace, COND_SkipOwner);
}

// Sets default values
AShooterWeapon::AShooterWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";
	BaseDamage = 20.f;
	RateOfFire = 600; //bullets per minute
	BulletSpread = 2.0f;

	SetReplicates(true);
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
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

	//if this weapon is on an client, then trigger fire also on server
	if(Role < ROLE_Authority)
	{
		ServerFire();
	}

	//but fire anyway because you are a client
	if(AActor* owner = GetOwner())
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		owner->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		FVector shotDirection = eyeRotator.Vector();

		// bullet spread
		float halfRad = FMath::DegreesToRadians(BulletSpread);
		shotDirection = FMath::VRandCone(shotDirection, halfRad, halfRad);

		FVector traceEnd = eyeLocation + shotDirection * 10000;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(owner);
		queryParams.AddIgnoredActor(this);
		queryParams.bTraceComplex = true; //gives us the exact result because traces every triangle instead of a simple collider
		queryParams.bReturnPhysicalMaterial = true;

		FVector tracerEndPoint = traceEnd;
		EPhysicalSurface surfaceType = SurfaceType_Default;

		FHitResult hitResult;
		if(GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams))
		{
			//is blocking hit! -> process damage
			float actualDamage = BaseDamage;

			AActor* hitActor = hitResult.GetActor();

			surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

			if(surfaceType == SURFACE_FLESHVULNERABLE)
			{
				actualDamage *= 4.f;
			}
			UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shotDirection, hitResult, owner->GetInstigatorController(), this, DamageType);

			PlayImpactEffects(surfaceType, hitResult.ImpactPoint);

			tracerEndPoint = hitResult.ImpactPoint;
		}

		if(DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);
		}

		PlayFireEffects(tracerEndPoint);

		//if you are a character on or hosting the server then replicate your weapon effects to other players too
		if(HasAuthority())
		{
			HitScanTrace.TraceEnd = tracerEndPoint;
			HitScanTrace.SurfaceType = surfaceType;
		}

		lastFireTime = GetWorld()->TimeSeconds;
	}

}

void AShooterWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceEnd);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceEnd);
}

void AShooterWeapon::ServerFire_Implementation()
{
	Fire();
}

bool AShooterWeapon::ServerFire_Validate()
{
	return true;
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

void AShooterWeapon::PlayImpactEffects(EPhysicalSurface surfaceType, const FVector& impactPoint)
{
	UParticleSystem* selectedEffect = nullptr;
	switch (surfaceType)
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
		FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector shotDirection = impactPoint - muzzleLocation;
		shotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), selectedEffect, impactPoint, shotDirection.Rotation());
	}
}
