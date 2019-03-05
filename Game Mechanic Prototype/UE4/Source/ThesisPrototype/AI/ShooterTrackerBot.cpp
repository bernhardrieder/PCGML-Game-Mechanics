// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ShooterTrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/HealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Particles/ParticleSystem.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Pawns/ShooterCharacter.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ChangingGuns.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTackerBotDrawing(
	TEXT("Game.DebugTrackerBot"),
	DebugTrackerBotDrawing,
	TEXT("Draw Debug for Tracker Bot"),
	ECVF_Cheat
);

// Sets default values
AShooterTrackerBot::AShooterTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	meshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	meshComp->SetCanEverAffectNavigation(false);
	meshComp->SetSimulatePhysics(true);
	meshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = meshComp;

	healthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	healthComp->SetTeamNumber(TEAMNUMBER_BOT);

	sphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	sphereComp->SetSphereRadius(200);
	sphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	sphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	sphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	sphereComp->SetupAttachment(RootComponent);

	movementAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementAudioComp"));
	movementAudioComponent->SetupAttachment(RootComponent);

	movementForce = 1000.f;
	bUseVelocityChange = false;
	requiredDistanceToTarget = 100.f;
	explosionDamage = 60.f;
	damageRadius = 350.f;
	selfDamageInterval = 0.25f;
	maxPowerLevel = 4;
}

// Called when the game starts or when spawned
void AShooterTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	// find initial move to
	nextPathPoint = getNextPathPoint();

	FTimerHandle timerHandle_CheckPowerLevel;
	GetWorldTimerManager().SetTimer(timerHandle_CheckPowerLevel, this, &AShooterTrackerBot::onCheckNearbyBots, 1.f, true);

	if (!materialInstance)
	{
		materialInstance = meshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, meshComp->GetMaterial(0));
	}

	healthComp->OnHealthChangedEvent.AddDynamic(this, &AShooterTrackerBot::onHealthChanged);
}

FVector AShooterTrackerBot::getNextPathPoint()
{
	//hack to get player location
	AActor* bestTarget = nullptr;
	float nearestTargetDistance = FLT_MAX;

	for (FConstPawnIterator it = GetWorld()->GetPawnIterator(); it; ++it)
	{
		APawn* testPawn = it->Get();
		if (!testPawn || UHealthComponent::IsFriendly(this, testPawn))
		{
			continue;
		}
		UHealthComponent* healthComp = Cast<UHealthComponent>(testPawn->GetComponentByClass(UHealthComponent::StaticClass()));
		if (healthComp && healthComp->GetHealth() > 0.f)
		{
			const float distanceToPawn = FVector::Distance(this->GetActorLocation(), testPawn->GetActorLocation());
			if(distanceToPawn < nearestTargetDistance)
			{
				bestTarget = testPawn;
				nearestTargetDistance = distanceToPawn;
			}
		}
	}

	if(bestTarget)
	{
		UNavigationPath* navPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), bestTarget);

		GetWorldTimerManager().ClearTimer(timerHandle_refreshPath);
		GetWorldTimerManager().SetTimer(timerHandle_refreshPath, this, &AShooterTrackerBot::refreshPath, 2.5f, false);

		if (navPath && navPath->PathPoints.Num() > 1)
		{
			//return next point in the path
			return navPath->PathPoints[1];
		}
	}

	//Failed to find path
	return GetActorLocation();
}

void AShooterTrackerBot::onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(materialInstance)
	{
		materialInstance->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}
	if (Health <= 0.f)
	{
		selfDestruct();
	}
}

void AShooterTrackerBot::selfDestruct()
{
	if(bExploded)
	{
		return;
	}

	bExploded = true;
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), explosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, explosionSound, GetActorLocation());

	meshComp->SetVisibility(false, true);
	meshComp->SetSimulatePhysics(false);
	meshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TArray<AActor*> ignoreDamageActors{ this };

	const float actualDamage = explosionDamage + (explosionDamage * currentPowerLevel);

	UGameplayStatics::ApplyRadialDamage(GetWorld(), actualDamage, GetActorLocation(), damageRadius, damageType, ignoreDamageActors, this, GetInstigatorController(), true);

	if (DebugTrackerBotDrawing > 0)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), damageRadius, 12, FColor::Red, false, 2.f, 0, 2.f);
	}

	SetLifeSpan(2.0f);
}

void AShooterTrackerBot::damageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void AShooterTrackerBot::onCheckNearbyBots()
{
	const float checkRadius = 600;

	if (DebugTrackerBotDrawing > 0)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), checkRadius, 12, FColor::White, false, 1.f);
	}

	int32 numOfNearbyBots = 0;

	FCollisionShape collShape;
	collShape.SetSphere(checkRadius);

	FCollisionObjectQueryParams queryParams;
	queryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	queryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> overlaps;
	GetWorld()->OverlapMultiByObjectType(overlaps, GetActorLocation(), FQuat::Identity, queryParams, collShape);
	for(const FOverlapResult& overlap : overlaps)
	{
		AShooterTrackerBot* bot = Cast<AShooterTrackerBot>(overlap.GetActor());
		if(bot && bot != this)
		{
			++numOfNearbyBots;
		}
	}

	currentPowerLevel = FMath::Clamp(numOfNearbyBots, 0, maxPowerLevel);

	if (materialInstance)
	{
		float alpha = currentPowerLevel / static_cast<float>(maxPowerLevel);
		materialInstance->SetScalarParameterValue("PowerLevelAlpha", alpha);
	}

	if (DebugTrackerBotDrawing > 0)
	{
		DrawDebugString(GetWorld(), GetActorLocation(), FString::FromInt(currentPowerLevel), this, FColor::White, 1.f, true);
	}
}

void AShooterTrackerBot::refreshPath()
{
	nextPathPoint = getNextPathPoint();
}

// Called every frame
void AShooterTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!bExploded)
	{
		float distanceToTarget = (GetActorLocation() - nextPathPoint).Size();
		if (distanceToTarget <= requiredDistanceToTarget)
		{
			nextPathPoint = getNextPathPoint();
		}
		else
		{
			//keep moving towards target
			FVector forceDirection = nextPathPoint - GetActorLocation();
			forceDirection.Normalize();
			forceDirection *= movementForce;

			meshComp->AddForce(forceDirection, NAME_None, bUseVelocityChange);

			if (DebugTrackerBotDrawing > 0)
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + forceDirection, 32, FColor::Green, false, 0.f, 0, 1.f);
			}
		}

		if (DebugTrackerBotDrawing > 0)
		{
			DrawDebugSphere(GetWorld(), nextPathPoint, 20, 12, FColor::Green, false, 0.f, 1.f);
		}
	}

	// movement sound
	float velocity = GetVelocity().Size();
	float volumeMultiplier = UKismetMathLibrary::MapRangeClamped(velocity, 10, 1000, 0.1, 2);
	movementAudioComponent->SetVolumeMultiplier(volumeMultiplier);
}

void AShooterTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(!bStartedSelfDestruction && !bExploded)
	{
		AShooterCharacter* shooterChar = Cast<AShooterCharacter>(OtherActor);
		if (shooterChar && !UHealthComponent::IsFriendly(OtherActor, this))
		{
			//Start self destruction sequence
			GetWorldTimerManager().SetTimer(timerHandle_selfDamage, this, &AShooterTrackerBot::damageSelf, selfDamageInterval, true, 0.0f);

			bStartedSelfDestruction = true;

			UGameplayStatics::SpawnSoundAttached(selfDestructSound, RootComponent);
		}
	}
}
