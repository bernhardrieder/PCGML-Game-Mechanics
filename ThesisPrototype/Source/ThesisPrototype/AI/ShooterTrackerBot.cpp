// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ShooterTrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "AI/Navigation/NavigationSystem.h"
#include "AI/Navigation/NavigationPath.h"
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

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;	

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	MovementAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementAudioComp"));
	MovementAudioComponent->SetupAttachment(RootComponent);

	MovementForce = 1000.f;
	bUseVelocityChange = false;
	RequiredDistanceToTarget = 100.f;
	ExplosionDamage = 60.f;
	DamageRadius = 350.f;
	SelfDamageInterval = 0.25f;
	MaxPowerLevel = 4;
}

// Called when the game starts or when spawned
void AShooterTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		// find initial move to
		nextPathPoint = GetNextPathPoint();

		FTimerHandle timerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(timerHandle_CheckPowerLevel, this, &AShooterTrackerBot::onCheckNearbyBots, 1.f, true);
	}

	if (!materialInstance)
	{
		materialInstance = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	HealthComp->OnHealthChangedEvent.AddDynamic(this, &AShooterTrackerBot::onHealthChanged);
}

FVector AShooterTrackerBot::GetNextPathPoint()
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
		UNavigationPath* navPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), bestTarget);

		GetWorldTimerManager().ClearTimer(timerHandle_RefreshPath);
		GetWorldTimerManager().SetTimer(timerHandle_RefreshPath, this, &AShooterTrackerBot::refreshPath, 2.5f, false);

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
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if(HasAuthority())
	{
		TArray<AActor*> ignoreDamageActors{ this };

		const float actualDamage = ExplosionDamage + (ExplosionDamage * currentPowerLevel);

		UGameplayStatics::ApplyRadialDamage(GetWorld(), actualDamage, GetActorLocation(), DamageRadius, DamageType, ignoreDamageActors, this, GetInstigatorController(), true);

		if (DebugTrackerBotDrawing > 0)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 12, FColor::Red, false, 2.f, 0, 2.f);
		}

		SetLifeSpan(2.0f);
	}
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

	currentPowerLevel = FMath::Clamp(numOfNearbyBots, 0, MaxPowerLevel);

	if (materialInstance)
	{
		float alpha = currentPowerLevel / (float)MaxPowerLevel;
		materialInstance->SetScalarParameterValue("PowerLevelAlpha", alpha);
	}

	if (DebugTrackerBotDrawing > 0)
	{
		DrawDebugString(GetWorld(), GetActorLocation(), FString::FromInt(currentPowerLevel), this, FColor::White, 1.f, true);
	}
}

void AShooterTrackerBot::refreshPath()
{
	nextPathPoint = GetNextPathPoint();
}

// Called every frame
void AShooterTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(HasAuthority() && !bExploded)
	{
		float distanceToTarget = (GetActorLocation() - nextPathPoint).Size();
		if (distanceToTarget <= RequiredDistanceToTarget)
		{
			nextPathPoint = GetNextPathPoint();
		}
		else
		{
			//keep moving towards target
			FVector forceDirection = nextPathPoint - GetActorLocation();
			forceDirection.Normalize();
			forceDirection *= MovementForce;

			MeshComp->AddForce(forceDirection, NAME_None, bUseVelocityChange);

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
	MovementAudioComponent->SetVolumeMultiplier(volumeMultiplier);
}

void AShooterTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(!bStartedSelfDestruction && !bExploded)
	{
		AShooterCharacter* shooterChar = Cast<AShooterCharacter>(OtherActor);
		if (shooterChar && !UHealthComponent::IsFriendly(OtherActor, this))
		{
			if(HasAuthority())
			{
				//Start self destruction sequence
				GetWorldTimerManager().SetTimer(timerHandle_SelfDamage, this, &AShooterTrackerBot::damageSelf, SelfDamageInterval, true, 0.0f);
			}

			bStartedSelfDestruction = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}
}
