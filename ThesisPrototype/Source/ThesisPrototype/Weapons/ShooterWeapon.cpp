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
#include "Curves/CurveFloat.h"

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
	RateOfFire = 600; //bullets per minute
	BulletSpread = 2.0f;
	AvailableMagazines = 3;
	BulletsPerMagazine = 30;
	BulletsInOneShot = 1;
	ReloadTimeEmptyMagazine = 3.f;
	FireMode = EFireMode::Automatic;
	bUnlimitiedBullets = false;

	DamageCurve.GetRichCurve()->AddKey(1000.f, 20.f);
	DamageCurve.GetRichCurve()->AddKey(10000.f, 5.f);

	m_singleBulletReloadTime = ReloadTimeEmptyMagazine / BulletsPerMagazine;

	Type = EWeaponType::Rifle;

	m_walkinSpeedModifier = 1.f;
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	timeBetweenShots = 60.f / RateOfFire;
	m_currentBulletsInMagazine = BulletsPerMagazine;
	m_availableBulletsLeft = AvailableMagazines * BulletsPerMagazine;
}

void AShooterWeapon::StartFire()
{
	if (m_bIsReloading) 
	{
		return;
	}
	float firstDelay = FMath::Max(lastFireTime + timeBetweenShots - GetWorld()->TimeSeconds, 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle_AutomaticFire, this, &AShooterWeapon::Fire, timeBetweenShots, true, firstDelay);
}

void AShooterWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_AutomaticFire);
}

void AShooterWeapon::StartMagazineReloading()
{
	if(m_bIsReloading || m_availableBulletsLeft <= 0)
	{
		return;
	}
	m_bIsReloading = true;
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadStock);
	const int bulletDifference = BulletsPerMagazine - m_currentBulletsInMagazine;
	const float reloadTimeNeeded = bulletDifference == BulletsPerMagazine ? ReloadTimeEmptyMagazine : m_singleBulletReloadTime * bulletDifference;
	GetWorldTimerManager().SetTimer(TimerHandle_ReloadMagazine, this, &AShooterWeapon::reloadMagazine, reloadTimeNeeded);
	OnReloadStateChangedEvent.Broadcast(m_bIsReloading, reloadTimeNeeded, m_currentBulletsInMagazine);
}

void AShooterWeapon::reloadMagazine()
{
	const int bulletDifference = BulletsPerMagazine - m_currentBulletsInMagazine;
	m_currentBulletsInMagazine = m_availableBulletsLeft >= BulletsPerMagazine ? BulletsPerMagazine : m_availableBulletsLeft;
	if(!bUnlimitiedBullets)
	{
		m_availableBulletsLeft -= bulletDifference;
	}

	OnAmmoChangedEvent.Broadcast(m_availableBulletsLeft, m_currentBulletsInMagazine);
	m_bIsAmmoLeftInMagazine = m_currentBulletsInMagazine > 0;
	m_bIsReloading = false;
	OnReloadStateChangedEvent.Broadcast(m_bIsReloading, 0.f, m_currentBulletsInMagazine);
}

void AShooterWeapon::startStockReloading()
{
	m_bIsReloading = true;
	GetWorldTimerManager().SetTimer(TimerHandle_ReloadStock, this, &AShooterWeapon::reloadStock, m_singleBulletReloadTime);
	OnReloadStateChangedEvent.Broadcast(m_bIsReloading, m_singleBulletReloadTime, m_currentBulletsInMagazine);
}

void AShooterWeapon::reloadStock()
{
	m_bIsReloading = false;
	OnReloadStateChangedEvent.Broadcast(m_bIsReloading, 0.f, m_currentBulletsInMagazine);
}

float AShooterWeapon::getDamageMultiplierFor(EPhysicalSurface surfaceType)
{
	switch (surfaceType)
	{
		case SURFACE_FLESHLOWERBOYDANDARMS: return 0.85f;
		case SURFACE_FLESHUPPERBODY: return 1.f;
		case SURFACE_FLESHHEAD: return 1.8f;
		case SURFACE_FLESHDEFAULT:
		default: return 1.f;
	}
}

void AShooterWeapon::Equip()
{
}

void AShooterWeapon::Disarm()
{
	StopFire();
	GetWorldTimerManager().ClearTimer(TimerHandle_AutomaticFire);
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadMagazine);
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadStock);
	m_bIsReloading = false;
}

void AShooterWeapon::Fire()
{
	if(!m_bIsAmmoLeftInMagazine)
	{
		//play some 'click click click' out of ammo sound
		StopFire();
		return;
	}
	//trace the world, from pawn eyes to crosshair location

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
			float actualDamage = DamageCurve.GetRichCurveConst()->Eval(hitResult.Distance);

			AActor* hitActor = hitResult.GetActor();

			surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

			actualDamage *= getDamageMultiplierFor(surfaceType);
			UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shotDirection, hitResult, owner->GetInstigatorController(), owner, DamageType);

			PlayImpactEffects(surfaceType, hitResult.ImpactPoint);

			tracerEndPoint = hitResult.ImpactPoint;
		}

		if(DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);
		}

		PlayFireEffects(tracerEndPoint);

		lastFireTime = GetWorld()->TimeSeconds;

		if(!bUnlimitiedBullets)
		{
			--m_currentBulletsInMagazine;
		}
		m_bIsAmmoLeftInMagazine = m_currentBulletsInMagazine > 0;
		OnAmmoChangedEvent.Broadcast(m_availableBulletsLeft, m_currentBulletsInMagazine);

		if (FireMode == EFireMode::SemiAutomatic || FireMode == EFireMode::SingleFire)
		{
			StopFire();
		}

		if (m_bIsAmmoLeftInMagazine && FireMode == EFireMode::SingleFire)
		{
			startStockReloading();
		}
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

void AShooterWeapon::PlayImpactEffects(EPhysicalSurface surfaceType, const FVector& impactPoint)
{
	UParticleSystem* selectedEffect = nullptr;
	switch (surfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHLOWERBOYDANDARMS:
	case SURFACE_FLESHUPPERBODY:
	case SURFACE_FLESHHEAD:
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
