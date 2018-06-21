// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Weapons/ShooterWeapon.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ChangingGuns.h"
#include "Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapons/WeaponGenerator.h"

// Sets default values
AShooterCharacter::AShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->bUsePawnControlRotation = true;
	springArmComp->SetupAttachment(RootComponent);

	healthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	cameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	cameraComp->SetupAttachment(springArmComp);

	zoomedFOV = 65.0;
	zoomInterpSpeed = 20.0f;
}

FVector AShooterCharacter::GetPawnViewLocation() const
{
	if(cameraComp)
	{
		return cameraComp->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	maxWalkSpeedDefault = GetCharacterMovement()->MaxWalkSpeed;
	maxWalkSpeedCrouchedDefault = GetCharacterMovement()->MaxWalkSpeedCrouched;

	defaultFOV = cameraComp->FieldOfView;
	healthComp->OnHealthChangedEvent.AddDynamic(this, &AShooterCharacter::onHealthChanged);

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for(const TSubclassOf<AShooterWeapon> weaponClass : starterWeaponClasses)
	{
		AShooterWeapon* starterWeapon = GetWorld()->SpawnActor<AShooterWeapon>(weaponClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
		addWeapon(starterWeapon);
	}

	if(bp_weaponGenerator.GetDefaultObject())
	{
		weaponGenerator = GetWorld()->SpawnActor<AWeaponGenerator>(bp_weaponGenerator, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
		weaponGenerator->SetOwner(this);
		weaponGenerator->OnWeaponGenerationFinishedEvent.AddDynamic(this, &AShooterCharacter::onNewWeaponGenerated);
		OnWeaponGeneratorAvailableEvent.Broadcast(weaponGenerator);
	}
	
	//(that'd be an axis input actually)
	switchWeapon(1.0f);
}

FName AShooterCharacter::getSocketNameFor(const AShooterWeapon* Weapon) const
{
	if (!Weapon)
		return FName("");

	//do it like that because I don't like to fiddle around with all of those 3d models 
	switch(Weapon->GetType())
	{
		case EWeaponType::Pistol: return FName("PistolSocket");
		case EWeaponType::SubMachineGun: return FName("SMGSocket");
		case EWeaponType::HeavyMachineGun: return FName("MGSocket");
		case EWeaponType::SniperRifle: return FName("SniperRifleSocket");
		case EWeaponType::Shotgun: return FName("ShotgunSocket");
		case EWeaponType::Rifle: 
		default: return FName("WeaponSocket");
	}
}


void AShooterCharacter::moveForward(float Value)
{
	AddMovementInput(GetActorForwardVector()*Value);
}

void AShooterCharacter::moveRight(float Value)
{
	AddMovementInput(GetActorRightVector()*Value);
}

void AShooterCharacter::beginCrouch()
{
	Crouch();
}

void AShooterCharacter::endCrouch()
{
	UnCrouch();
}

void AShooterCharacter::beginZoom()
{
	bWantsToZoom = true;
}

void AShooterCharacter::endZoom()
{
	bWantsToZoom = false;
}

void AShooterCharacter::beginRun()
{
	if(auto movementComp = GetCharacterMovement())
	{
		movementComp->MaxWalkSpeed *= 1.5f;
	}
}

void AShooterCharacter::endRun()
{
	if (auto movementComp = GetCharacterMovement())
	{
		movementComp->MaxWalkSpeed /= 1.5f;
	}
}

void AShooterCharacter::reloadWeapon()
{
	if(equippedWeapon)
	{
		equippedWeapon->StartMagazineReloading();
	}
}

void AShooterCharacter::equipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if(equippedWeapon && lastEquippedWeapon != equippedWeapon)
		{
			lastEquippedWeapon = equippedWeapon;
		}
		equippedWeapon = Weapon;
		equippedWeapon->Equip(this);
		equippedWeapon->SetActorHiddenInGame(false);
		equippedWeapon->SetActorEnableCollision(true);
		OnCurrentWeaponChangedEvent.Broadcast(equippedWeapon);
		GetCharacterMovement()->MaxWalkSpeed = maxWalkSpeedDefault * Weapon->GetWalkinSpeedModifier();
		GetCharacterMovement()->MaxWalkSpeedCrouched = maxWalkSpeedCrouchedDefault * Weapon->GetWalkinSpeedModifier();
	}
}

void AShooterCharacter::disarmWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		Weapon->Disarm();
		Weapon->SetActorHiddenInGame(true);
		Weapon->SetActorEnableCollision(false);
	}
}

void AShooterCharacter::switchWeapon(float Value)
{
	if (!FMath::IsNearlyZero(Value) && availableWeapons.Num() > 0)
	{
		int32 nextIdx = 0;
		if (equippedWeapon)
		{
			bool nextWeapon = Value >= 0.f;
			int32 idx = availableWeapons.Find(equippedWeapon);
			if(nextWeapon)
			{
				++idx;
				nextIdx = idx >= availableWeapons.Num() ? 0 : idx;
			}
			else
			{
				--idx;
				nextIdx = idx < 0 ? availableWeapons.Num() - 1 : idx;
			}
			if(equippedWeapon == availableWeapons[nextIdx])
			{
				//do nothing!
				return;
			}
			disarmWeapon(equippedWeapon);
		}
		equipWeapon(availableWeapons[nextIdx]);
	}
}

void AShooterCharacter::switchToLastEquipedWeapon()
{
	if(lastEquippedWeapon && lastEquippedWeapon != equippedWeapon)
	{
		disarmWeapon(equippedWeapon);
		equipWeapon(lastEquippedWeapon);
	}
}

void AShooterCharacter::StartFire()
{
	if(equippedWeapon)
	{
		equippedWeapon->StartFire();
	}
}

void AShooterCharacter::StopFire()
{
	if (equippedWeapon)
	{
		equippedWeapon->StopFire();
	}
}

bool AShooterCharacter::IsMoving() const
{
	return GetCharacterMovement()->GetCurrentAcceleration() != FVector::ZeroVector;
}

bool AShooterCharacter::IsCrouching() const
{
	return GetCharacterMovement()->IsCrouching();
}

void AShooterCharacter::addWeapon(AShooterWeapon* Weapon)
{
	if(Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, getSocketNameFor(Weapon));
		disarmWeapon(Weapon);
		availableWeapons.Add(Weapon);
	}

}

void AShooterCharacter::removeWeapon(AShooterWeapon* Weapon)
{
	if(Weapon && Weapon != equippedWeapon)
	{
		availableWeapons.Remove(Weapon);
		if(!Weapon->IsPendingKill())
		{
			Weapon->Destroy();
		}
	}
}

void AShooterCharacter::dismantleEquippedWeaponAndGenerateNew()
{
	if (weaponGenerator->IsGenerating() || !weaponGenerator->IsReadyToUse())
		return;

	AShooterWeapon* dismantle = equippedWeapon;
	switchWeapon(1.f);
	lastEquippedWeapon = nullptr;

	weaponGenerator->DismantleWeapon(dismantle);
	removeWeapon(dismantle);
	dismantle = nullptr;
}

void AShooterCharacter::onNewWeaponGenerated(AShooterWeapon* Weapon)
{
	if (!Weapon)
		return;

	addWeapon(Weapon);
}

void AShooterCharacter::onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(Health <= 0.f && !bDied)
	{
		bDied = true;

		StopFire();
		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.f);

		for(AShooterWeapon* weapon : availableWeapons)
		{
			weapon->SetLifeSpan(10.f);
		}
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float targetFOV = bWantsToZoom ? zoomedFOV : defaultFOV;
	float newFOV = FMath::FInterpTo(cameraComp->FieldOfView, targetFOV, DeltaTime, zoomInterpSpeed);
	cameraComp->SetFieldOfView(newFOV);
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::moveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::moveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::beginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AShooterCharacter::endCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &AShooterCharacter::beginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &AShooterCharacter::endZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::StopFire);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AShooterCharacter::beginRun);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AShooterCharacter::endRun);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::reloadWeapon);

	PlayerInputComponent->BindAction("SwitchToLastEquippedWeapon", IE_Pressed, this, &AShooterCharacter::switchToLastEquipedWeapon);
	PlayerInputComponent->BindAxis("SwitchWeapon", this, &AShooterCharacter::switchWeapon);
	PlayerInputComponent->BindAction("DismantleEquippedWeaponAndGenerateNew", IE_Pressed, this, &AShooterCharacter::dismantleEquippedWeaponAndGenerateNew);
}

