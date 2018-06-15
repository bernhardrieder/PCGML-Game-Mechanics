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


float FOwnerBasedModifier::GetCurrentModifier(AShooterCharacter* Character)
{
	float modifier = 1.f;
	modifier *= Character->IsCrouching() ? Crouching : 1.f;
	modifier *= Character->IsMoving() ? Moving : 1.f;
	modifier += Character->IsAiming() ? Aiming : 1.f;
	return modifier;
}

// Sets default values
AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	meshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = meshComp;

	muzzleSocketName = "MuzzleSocket";
	tracerTargetName = "BeamEnd";
	rateOfFire = 600; //bullets per minute
	availableMagazines = 3;
	bulletsPerMagazine = 30;
	bulletsInOneShot = 1;
	reloadTimeEmptyMagazine = 3.f;
	fireMode = EFireMode::Automatic;
	bUnlimitiedBullets = false;

	maxDamageWithDistance = FVector2D(20.f, 1000.f); // 10m
	minDamageWithDistance = FVector2D(5.f, 10000.f); // 5m

	type = EWeaponType::Rifle;

	walkinSpeedModifier = 1.f;
	bulletSpreadIncrease = 0.1f;
	bulletSpreadDecrease = 4.f;

	spreadModifier.Moving = 1.3f;
	spreadModifier.Crouching = 0.5f;
	spreadModifier.Aiming = 0.2;

	recoilModifier.Moving = 1.3f;
	recoilModifier.Crouching = 0.5f;
	recoilModifier.Aiming = 0.2f;

	recoilIncreasePerShot = FVector2D(0.4f, 1.5f);
	recoilDecrease = 3.f;
}

void AShooterWeapon::SetMaxDamageWithDistance(const FVector2D& MaxDamageWithDistance)
{
	maxDamageWithDistance = MaxDamageWithDistance;
	buildDamageCurve();
}

void AShooterWeapon::SetMinDamageWithDistance(const FVector2D& MinDamageWithDistance)
{
	minDamageWithDistance = MinDamageWithDistance;
	buildDamageCurve();
}

void AShooterWeapon::SetRateOfFire(int32 RateOfFire)
{
	rateOfFire = RateOfFire;
	timeBetweenShots = 60.f / rateOfFire;
}

void AShooterWeapon::SetBulletsPerMagazine(int32 Bullets)
{
	bulletsPerMagazine = Bullets;
	currentBulletsInMagazine = bulletsPerMagazine;
	availableBulletsLeft = availableMagazines * bulletsPerMagazine;
	updateSingleBulletReloadTime();
	OnAmmoChangedEvent.Broadcast(availableBulletsLeft, currentBulletsInMagazine);
}

void AShooterWeapon::SetReloadTimeEmptyMagazine(float Time)
{
	reloadTimeEmptyMagazine = Time;
	updateSingleBulletReloadTime();
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetRateOfFire(rateOfFire);
	SetBulletsPerMagazine(bulletsPerMagazine);
	buildDamageCurve();
	updateSingleBulletReloadTime();
}

void AShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	compensateRecoil(DeltaTime);
}

void AShooterWeapon::StartFire()
{
	if (bIsReloading) 
	{
		return;
	}
	float firstDelay = FMath::Max(lastFireTime + timeBetweenShots - GetWorld()->TimeSeconds, 0.f);
	GetWorldTimerManager().SetTimer(timerHandle_AutomaticFire, this, &AShooterWeapon::fire, timeBetweenShots, true, firstDelay);
}

void AShooterWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(timerHandle_AutomaticFire);
}

void AShooterWeapon::StartMagazineReloading()
{
	if(bIsReloading || availableBulletsLeft <= 0 || currentBulletsInMagazine == bulletsPerMagazine)
	{
		return;
	}
	bIsReloading = true;
	GetWorldTimerManager().ClearTimer(timerHandle_ReloadStock);
	const int bulletDifference = bulletsPerMagazine - currentBulletsInMagazine;
	const float reloadTimeNeeded = bulletDifference == bulletsPerMagazine ? reloadTimeEmptyMagazine : singleBulletReloadTime * bulletDifference;
	GetWorldTimerManager().SetTimer(timerHandle_ReloadMagazine, this, &AShooterWeapon::reloadMagazine, reloadTimeNeeded);
	OnReloadStateChangedEvent.Broadcast(bIsReloading, reloadTimeNeeded, currentBulletsInMagazine);
}

void AShooterWeapon::Equip(AShooterCharacter* EuqippedBy)
{
	owningCharacter = EuqippedBy;
	timeEquipped = GetWorld()->TimeSeconds;
}

void AShooterWeapon::Disarm()
{
	StopFire();
	GetWorldTimerManager().ClearTimer(timerHandle_AutomaticFire);
	GetWorldTimerManager().ClearTimer(timerHandle_ReloadMagazine);
	GetWorldTimerManager().ClearTimer(timerHandle_ReloadStock);
	GetWorldTimerManager().ClearTimer(timerHandle_SpreadDecrease);
	bIsReloading = false;
	currentBulletSpread = 0.f;
	currentRecoil = FVector2D::ZeroVector;

	if(!FMath::IsNearlyZero(timeEquipped,0.5f))
	{
		statistics.SecondsUsed += GetWorld()->TimeSeconds - timeEquipped;
	}

	owningCharacter = nullptr;
	PrimaryActorTick.SetTickFunctionEnable(false);

}

void AShooterWeapon::RefillAmmunition(int AmountOfBullets)
{
	int maxBullets = availableMagazines * bulletsPerMagazine;
	int maxNewBullets = maxBullets - availableBulletsLeft;
	availableBulletsLeft += AmountOfBullets < maxNewBullets ? AmountOfBullets : maxNewBullets;
	OnAmmoChangedEvent.Broadcast(availableBulletsLeft, currentBulletsInMagazine);
}

void AShooterWeapon::applyRecoil()
{
	if (FMath::IsNearlyZero(recoilIncreasePerShot.Y) && FMath::IsNearlyZero(recoilIncreasePerShot.X))
		return;

	FVector2D recoil;
	
	recoil.Y = -recoilIncreasePerShot.Y;
	recoil.X = FMath::FRandRange(-recoilIncreasePerShot.X, recoilIncreasePerShot.X);
	recoil *= recoilModifier.GetCurrentModifier(owningCharacter);

	owningCharacter->AddControllerPitchInput(recoil.Y);
	owningCharacter->AddControllerYawInput(recoil.X);

	currentRecoil += recoil;

	PrimaryActorTick.SetTickFunctionEnable(true);
}

float AShooterWeapon::calculateRecoilCompensationDelta(float DeltaTime, float CurrentRecoil)
{
	//calculations according to http://symthic.com/bf1-general-info?p=misc
	//C = Some constant(approx. 5.0)
	const float magicConstant = 5.0f;

	//RecoilTerm = ((abs(CurrentRecoil) / 0.5) ^ 0.6 + .001)
	const float recoilTerm = FMath::Pow(FMath::Abs(CurrentRecoil) / 0.5f, 0.6) + 0.001f;

	//Decrease = RecoilTerm * RecoilDecrease * DeltaTime * TimeSinceLastShot^0.5 * C
	const float timeSinceLastShot = GetWorld()->TimeSeconds - lastFireTime;
	float delta = recoilTerm * recoilDecrease * recoilModifier.GetCurrentModifier(owningCharacter) * DeltaTime * FMath::Pow(timeSinceLastShot, 0.5f) * magicConstant;

	delta *= CurrentRecoil > 0.f ? -1.f : 1.f;

	return delta;
}

void AShooterWeapon::compensateRecoil(float DeltaTime)
{
	FVector2D recoilDelta;
	recoilDelta.X = calculateRecoilCompensationDelta(DeltaTime, currentRecoil.X);
	recoilDelta.Y = calculateRecoilCompensationDelta(DeltaTime, currentRecoil.Y);

	currentRecoil += recoilDelta;

	owningCharacter->AddControllerYawInput(recoilDelta.X);
	owningCharacter->AddControllerPitchInput(recoilDelta.Y);

	if(FMath::IsNearlyZero(currentRecoil.X, .25f) && FMath::IsNearlyZero(currentRecoil.Y, .25f))
	{
		currentRecoil = FVector2D::ZeroVector;
		PrimaryActorTick.SetTickFunctionEnable(false);
	}
}

void AShooterWeapon::reloadMagazine()
{
	const int bulletDifference = bulletsPerMagazine - currentBulletsInMagazine;
	currentBulletsInMagazine = availableBulletsLeft >= bulletsPerMagazine ? bulletsPerMagazine : availableBulletsLeft;
	if(!bUnlimitiedBullets)
	{
		availableBulletsLeft -= bulletDifference;
	}

	OnAmmoChangedEvent.Broadcast(availableBulletsLeft, currentBulletsInMagazine);
	bIsAmmoLeftInMagazine = currentBulletsInMagazine > 0;
	bIsReloading = false;
	OnReloadStateChangedEvent.Broadcast(bIsReloading, 0.f, currentBulletsInMagazine);
}

void AShooterWeapon::startStockReloading()
{
	bIsReloading = true;
	GetWorldTimerManager().SetTimer(timerHandle_ReloadStock, this, &AShooterWeapon::reloadStock, singleBulletReloadTime);
	OnReloadStateChangedEvent.Broadcast(bIsReloading, singleBulletReloadTime, currentBulletsInMagazine);
}

void AShooterWeapon::reloadStock()
{
	bIsReloading = false;
	OnReloadStateChangedEvent.Broadcast(bIsReloading, 0.f, currentBulletsInMagazine);
}

void AShooterWeapon::decreaseBulletSpread()
{
	currentBulletSpread -= bulletSpreadDecrease;
	if(currentBulletSpread <= 0)
	{
		currentBulletSpread = 0;
		GetWorldTimerManager().ClearTimer(timerHandle_SpreadDecrease);
	}
}

float AShooterWeapon::getDamageMultiplierFor(EPhysicalSurface SurfaceType)
{
	switch (SurfaceType)
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
	damageCurve.GetRichCurve()->Reset();
	damageCurve.GetRichCurve()->AddKey(0.f, maxDamageWithDistance.X);
	damageCurve.GetRichCurve()->AddKey(maxDamageWithDistance.Y, maxDamageWithDistance.X);
	damageCurve.GetRichCurve()->AddKey(minDamageWithDistance.Y, minDamageWithDistance.X);
}

void AShooterWeapon::updateSingleBulletReloadTime()
{
	singleBulletReloadTime = reloadTimeEmptyMagazine / bulletsPerMagazine;
}

FVector2D AShooterWeapon::calculateBulletSpreadDispersion(float RandomPower, float CurrentSpread)
{
	const float random = FMath::FRandRange(0.f, 1.f);
	const float randomSinCos = FMath::FRandRange(0.f, 2 * PI);
	const float randomPowered = FMath::Pow(random, RandomPower);
	const float horizontalDispersion = randomPowered * CurrentSpread * FMath::Cos(randomSinCos);
	const float verticalDispersion = randomPowered * CurrentSpread * FMath::Sin(randomSinCos);
	FVector2D spreadDispersion = FVector2D(horizontalDispersion, verticalDispersion);
	spreadDispersion *= spreadModifier.GetCurrentModifier(owningCharacter);
	return spreadDispersion;
}


void AShooterWeapon::fire()
{
	if(!bIsAmmoLeftInMagazine)
	{
		//play some 'click click click' out of ammo sound
		StopFire();
		return;
	}
	//trace the world, from pawn eyes to crosshair location

	//but fire anyway because you are a client
	if(owningCharacter)
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		owningCharacter->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		FVector shotDirection = eyeRotator.Vector();


		//bullet spread calculations according to http://symthic.com/bf1-general-info?p=misc
		if(currentBulletSpread > 0.f)
		{
			const FVector2D spread = calculateBulletSpreadDispersion(type == EWeaponType::Shotgun ? 1.0f : 0.5f, currentBulletSpread);
			shotDirection.Y += spread.X;
			shotDirection.Z += spread.Y;
			shotDirection.Normalize();
		}
		currentBulletSpread += bulletSpreadIncrease;
		if (!GetWorldTimerManager().IsTimerActive(timerHandle_SpreadDecrease))
		{
			GetWorldTimerManager().SetTimer(timerHandle_SpreadDecrease, this, &AShooterWeapon::decreaseBulletSpread, 1.f, true, timeBetweenShots);
		}

		for(int32 i = 0; i < bulletsInOneShot; ++i)
		{
			FVector bulletShotDirection = shotDirection;
			if(bulletsInOneShot > 1)
			{
				const FVector2D spread = calculateBulletSpreadDispersion(1.0f, FMath::FRandRange(0.001f, 0.1f));
				bulletShotDirection.Y += spread.X;
				bulletShotDirection.Z += spread.Y;
				bulletShotDirection.Normalize();
			}
			FVector traceEnd = eyeLocation + bulletShotDirection * 10000;

			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(owningCharacter);
			queryParams.AddIgnoredActor(this);
			queryParams.bTraceComplex = true; //gives us the exact result because traces every triangle instead of a simple collider
			queryParams.bReturnPhysicalMaterial = true;

			FVector tracerEndPoint = traceEnd;
			EPhysicalSurface surfaceType = SurfaceType_Default;

			FHitResult hitResult;
			if (GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, COLLISION_WEAPON, queryParams))
			{
				//is blocking hit! -> process damage
				float actualDamage = damageCurve.GetRichCurveConst()->Eval(hitResult.Distance);

				AActor* hitActor = hitResult.GetActor();

				surfaceType = UPhysicalMaterial::DetermineSurfaceType(hitResult.PhysMaterial.Get());

				actualDamage *= getDamageMultiplierFor(surfaceType);

				if(UHealthComponent* healtComp = Cast<UHealthComponent>(hitActor->GetComponentByClass(UHealthComponent::StaticClass())))
				{
					UGameplayStatics::ApplyPointDamage(hitActor, actualDamage, shotDirection, hitResult, owningCharacter->GetInstigatorController(), owningCharacter, damageType);
					if(healtComp->GetHealth() <= 0)
					{
						++statistics.Kills;
					}
				}

				playImpactEffects(surfaceType, hitResult.ImpactPoint);

				tracerEndPoint = hitResult.ImpactPoint;
			}

			if (DebugWeaponDrawing > 0)
			{
				DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);
			}

			playFireEffects(tracerEndPoint);
		}

		UGameplayStatics::PlaySoundAtLocation(this, fireSound, GetActorLocation());

		applyRecoil();

		lastFireTime = GetWorld()->TimeSeconds;

		if(!bUnlimitiedBullets)
		{
			--currentBulletsInMagazine;
		}
		bIsAmmoLeftInMagazine = currentBulletsInMagazine > 0;
		OnAmmoChangedEvent.Broadcast(availableBulletsLeft, currentBulletsInMagazine);

		if (fireMode == EFireMode::SemiAutomatic || fireMode == EFireMode::SingleFire)
		{
			StopFire();
		}

		if (bIsAmmoLeftInMagazine && fireMode == EFireMode::SingleFire)
		{
			startStockReloading();
		}
	}

}

void AShooterWeapon::playFireEffects(const FVector& FireImpactPoint)
{
	if (muzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(muzzleEffect, meshComp, muzzleSocketName);
	}

	if (tracerEffect)
	{
		FVector muzzleLocation = meshComp->GetSocketLocation(muzzleSocketName);
		UParticleSystemComponent* particleSystem = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), tracerEffect, muzzleLocation);
		particleSystem->SetVectorParameter(tracerTargetName, FireImpactPoint);
	}

	if(APawn* owner = Cast<APawn>(GetOwner()))
	{
		if(APlayerController* pc = Cast<APlayerController>(owner->GetController()))
		{
			pc->ClientPlayCameraShake(fireCamShake);
		}
	}
}

void AShooterWeapon::playImpactEffects(EPhysicalSurface SurfaceType, const FVector& ImpactPoint)
{
	UParticleSystem* selectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHLOWERBOYDANDARMS:
	case SURFACE_FLESHUPPERBODY:
	case SURFACE_FLESHHEAD:
		selectedEffect = fleshImpactEffect;
		break;
	default:
		selectedEffect = defaultImpactEffect;
		break;
	}

	if (selectedEffect)
	{
		FVector muzzleLocation = meshComp->GetSocketLocation(muzzleSocketName);

		FVector shotDirection = ImpactPoint - muzzleLocation;
		shotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), selectedEffect, ImpactPoint, shotDirection.Rotation());
	}
}
