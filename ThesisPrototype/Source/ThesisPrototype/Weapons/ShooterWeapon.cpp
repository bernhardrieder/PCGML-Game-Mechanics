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
#include "GameFramework/Character.h"
#include "Pawns/ShooterCharacter.h"
#include "ChangingGunsPlayerState.h"
#include "Sound/SoundCue.h"
#include "Components/HealthComponent.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing (
	TEXT("Game.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat
);


float FOwnerBasedModifier::GetCurrentModifier(AShooterCharacter* character)
{
	float modifier = 1.f;
	modifier *= character->IsCrouching() ? Crouching : 1.f;
	modifier *= character->IsMoving() ? Moving : 1.f;
	modifier += character->IsAiming() ? Aiming : 1.f;
	return modifier;
}

// Sets default values
AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";
	RateOfFire = 600; //bullets per minute
	AvailableMagazines = 3;
	BulletsPerMagazine = 30;
	BulletsInOneShot = 1;
	ReloadTimeEmptyMagazine = 3.f;
	FireMode = EFireMode::Automatic;
	bUnlimitiedBullets = false;

	MaxDamageWithDistance = FVector2D(20.f, 1000.f); // 10m
	MinDamageWithDistance = FVector2D(5.f, 10000.f); // 5m

	Type = EWeaponType::Rifle;

	m_walkinSpeedModifier = 1.f;
	BulletSpreadIncrease = 0.1f;
	BulletSpreadDecrease = 4.f;

	m_spreadModifier.Moving = 1.3f;
	m_spreadModifier.Crouching = 0.5f;
	m_spreadModifier.Aiming = 0.2;

	m_recoilModifier.Moving = 1.3f;
	m_recoilModifier.Crouching = 0.5f;
	m_recoilModifier.Aiming = 0.2f;

	RecoilIncreasePerShot = FVector2D(0.4f, 1.5f);
	RecoilDecrease = 3.f;
}

void AShooterWeapon::SetMaxDamageWithDistance(const FVector2D& maxDamageWithDistance)
{
	MaxDamageWithDistance = maxDamageWithDistance;
	buildDamageCurve();
}

void AShooterWeapon::SetMinDamageWithDistance(const FVector2D& minDamageWithDistance)
{
	MinDamageWithDistance = minDamageWithDistance;
	buildDamageCurve();
}

void AShooterWeapon::SetRateOfFire(int32 rof)
{
	RateOfFire = rof;
	timeBetweenShots = 60.f / RateOfFire;
}

void AShooterWeapon::SetBulletsPerMagazine(int32 bullets)
{
	BulletsPerMagazine = bullets;
	m_currentBulletsInMagazine = BulletsPerMagazine;
	m_availableBulletsLeft = AvailableMagazines * BulletsPerMagazine;
	updateSingleBulletReloadTime();
	OnAmmoChangedEvent.Broadcast(m_availableBulletsLeft, m_currentBulletsInMagazine);
}

void AShooterWeapon::SetReloadTimeEmptyMagazine(float time)
{
	ReloadTimeEmptyMagazine = time;
	updateSingleBulletReloadTime();
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetRateOfFire(RateOfFire);
	SetBulletsPerMagazine(BulletsPerMagazine);
	buildDamageCurve();
	updateSingleBulletReloadTime();
}

void AShooterWeapon::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	compensateRecoil(deltaTime);
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
	if(m_bIsReloading || m_availableBulletsLeft <= 0 || m_currentBulletsInMagazine == BulletsPerMagazine)
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

void AShooterWeapon::Equip(AShooterCharacter* euqippedBy)
{
	m_owningCharacter = euqippedBy;
	m_timeEquipped = GetWorld()->TimeSeconds;
}

void AShooterWeapon::Disarm()
{
	StopFire();
	GetWorldTimerManager().ClearTimer(TimerHandle_AutomaticFire);
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadMagazine);
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadStock);
	GetWorldTimerManager().ClearTimer(TimerHandle_SpreadDecrease);
	m_bIsReloading = false;
	m_currentBulletSpread = 0.f;
	m_currentRecoil = FVector2D::ZeroVector;

	if(!FMath::IsNearlyZero(m_timeEquipped,0.5f))
	{
		m_statistics.SecondsUsed += GetWorld()->TimeSeconds - m_timeEquipped;
	}

	m_owningCharacter = nullptr;
	PrimaryActorTick.SetTickFunctionEnable(false);

}

void AShooterWeapon::RefillAmmunition(int amountOfBullets)
{
	int maxBullets = AvailableMagazines * BulletsPerMagazine;
	int maxNewBullets = maxBullets - m_availableBulletsLeft;
	m_availableBulletsLeft += amountOfBullets < maxNewBullets ? amountOfBullets : maxNewBullets;
	OnAmmoChangedEvent.Broadcast(m_availableBulletsLeft, m_currentBulletsInMagazine);
}

void AShooterWeapon::applyRecoil()
{
	FVector2D recoil;
	
	recoil.Y = -RecoilIncreasePerShot.Y;
	recoil.X = FMath::FRandRange(-RecoilIncreasePerShot.X, RecoilIncreasePerShot.X);
	recoil *= m_recoilModifier.GetCurrentModifier(m_owningCharacter);

	m_owningCharacter->AddControllerPitchInput(recoil.Y);
	m_owningCharacter->AddControllerYawInput(recoil.X);

	m_currentRecoil += recoil;

	PrimaryActorTick.SetTickFunctionEnable(true);
}

float AShooterWeapon::calculateRecoilCompensationDelta(float deltaTime, float currentRecoil)
{
	//calculations according to http://symthic.com/bf1-general-info?p=misc
	//C = Some constant(approx. 5.0)
	const float magicConstant = 5.0f;

	//RecoilTerm = ((abs(CurrentRecoil) / 0.5) ^ 0.6 + .001)
	const float recoilTerm = FMath::Pow(FMath::Abs(currentRecoil) / 0.5f, 0.6) + 0.001f;

	//Decrease = RecoilTerm * RecoilDecrease * DeltaTime * TimeSinceLastShot^0.5 * C
	const float timeSinceLastShot = GetWorld()->TimeSeconds - lastFireTime;
	float delta = recoilTerm * RecoilDecrease * m_recoilModifier.GetCurrentModifier(m_owningCharacter) * deltaTime * FMath::Pow(timeSinceLastShot, 0.5f) * magicConstant;

	delta *= currentRecoil > 0.f ? -1.f : 1.f;

	return delta;
}

void AShooterWeapon::compensateRecoil(float deltaTime)
{
	FVector2D recoilDelta;
	recoilDelta.X = calculateRecoilCompensationDelta(deltaTime, m_currentRecoil.X);
	recoilDelta.Y = calculateRecoilCompensationDelta(deltaTime, m_currentRecoil.Y);

	m_currentRecoil += recoilDelta;

	m_owningCharacter->AddControllerYawInput(recoilDelta.X);
	m_owningCharacter->AddControllerPitchInput(recoilDelta.Y);

	if(FMath::IsNearlyZero(m_currentRecoil.X, .25f) && FMath::IsNearlyZero(m_currentRecoil.Y, .25f))
	{
		m_currentRecoil = FVector2D::ZeroVector;
		PrimaryActorTick.SetTickFunctionEnable(false);
	}
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

void AShooterWeapon::decreaseBulletSpread()
{
	m_currentBulletSpread -= BulletSpreadDecrease;
	if(m_currentBulletSpread <= 0)
	{
		m_currentBulletSpread = 0;
		GetWorldTimerManager().ClearTimer(TimerHandle_SpreadDecrease);
	}
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

void AShooterWeapon::buildDamageCurve()
{
	m_damageCurve.GetRichCurve()->Reset();
	m_damageCurve.GetRichCurve()->AddKey(0.f, MaxDamageWithDistance.X);
	m_damageCurve.GetRichCurve()->AddKey(MaxDamageWithDistance.Y, MaxDamageWithDistance.X);
	m_damageCurve.GetRichCurve()->AddKey(MinDamageWithDistance.Y, MinDamageWithDistance.X);
}

void AShooterWeapon::updateSingleBulletReloadTime()
{
	m_singleBulletReloadTime = ReloadTimeEmptyMagazine / BulletsPerMagazine;
}

FVector2D AShooterWeapon::calculateBulletSpreadDispersion(float randomPower, float currentSpread)
{
	const float random = FMath::FRandRange(0.f, 1.f);
	const float randomSinCos = FMath::FRandRange(0.f, 2 * PI);
	const float randomPowered = FMath::Pow(random, randomPower);
	const float horizontalDispersion = randomPowered * currentSpread * FMath::Cos(randomSinCos);
	const float verticalDispersion = randomPowered * currentSpread * FMath::Sin(randomSinCos);
	FVector2D spreadDispersion = FVector2D(horizontalDispersion, verticalDispersion);
	spreadDispersion *= m_spreadModifier.GetCurrentModifier(m_owningCharacter);
	return spreadDispersion;
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
	if(m_owningCharacter)
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		m_owningCharacter->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		FVector shotDirection = eyeRotator.Vector();


		//bullet spread calculations according to http://symthic.com/bf1-general-info?p=misc
		if(m_currentBulletSpread > 0.f)
		{
			const FVector2D spread = calculateBulletSpreadDispersion(Type == EWeaponType::Shotgun ? 1.0f : 0.5f, m_currentBulletSpread);
			shotDirection.Y += spread.X;
			shotDirection.Z += spread.Y;
			shotDirection.Normalize();
		}
		m_currentBulletSpread += BulletSpreadIncrease;
		if (!GetWorldTimerManager().IsTimerActive(TimerHandle_SpreadDecrease))
		{
			GetWorldTimerManager().SetTimer(TimerHandle_SpreadDecrease, this, &AShooterWeapon::decreaseBulletSpread, 1.f, true, timeBetweenShots);
		}

		for(int32 i = 0; i < BulletsInOneShot; ++i)
		{
			FVector bulletShotDirection = shotDirection;
			if(BulletsInOneShot > 1)
			{
				const FVector2D spread = calculateBulletSpreadDispersion(1.0f, FMath::FRandRange(0.001f, 0.1f));
				bulletShotDirection.Y += spread.X;
				bulletShotDirection.Z += spread.Y;
				bulletShotDirection.Normalize();
			}
			FVector traceEnd = eyeLocation + bulletShotDirection * 10000;

			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(m_owningCharacter);
			queryParams.AddIgnoredActor(this);
			queryParams.bTraceComplex = true; //gives us the exact result because traces every triangle instead of a simple collider
			queryParams.bReturnPhysicalMaterial = true;

			FVector tracerEndPoint = traceEnd;
			EPhysicalSurface surfaceType = SurfaceType_Default;

			FHitResult hitResult;
			if (GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams))
			{
				//is blocking hit! -> process damage
				float actualDamage = m_damageCurve.GetRichCurveConst()->Eval(hitResult.Distance);

				AActor* hitActor = hitResult.GetActor();

				surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

				actualDamage *= getDamageMultiplierFor(surfaceType);

				if(UHealthComponent* healtComp = Cast<UHealthComponent>(hitActor->GetComponentByClass(UHealthComponent::StaticClass())))
				{
					UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shotDirection, hitResult, m_owningCharacter->GetInstigatorController(), m_owningCharacter, DamageType);
					if(healtComp->GetHealth() <= 0)
					{
						++m_statistics.Kills;
						UE_LOG(LogTemp, Log, TEXT("%s kills: %i"), *this->GetName(), m_statistics.Kills);
					}
				}

				PlayImpactEffects(surfaceType, hitResult.ImpactPoint);

				tracerEndPoint = hitResult.ImpactPoint;
			}

			if (DebugWeaponDrawing > 0)
			{
				DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);
			}

			PlayFireEffects(tracerEndPoint);
		}

		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());

		applyRecoil();

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
