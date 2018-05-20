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
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	MovementForce = 1000.f;
	bUseVelocityChange = false;
	RequiredDistanceToTarget = 100.f;
	ExplosionDamage = 40.f;
	DamageRadius = 200.f;
}

// Called when the game starts or when spawned
void AShooterTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	// find initial move to
	nextPathPoint = GetNextPathPoint();

	HealthComp->OnHealthChangedEvent.AddDynamic(this, &AShooterTrackerBot::onHealthChanged);
}

FVector AShooterTrackerBot::GetNextPathPoint()
{
	//hack to get player location
	ACharacter* playerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* navPath = UNavigationSystem::FindPathToActorSynchronously(this, GetActorLocation(), playerPawn);
	if(navPath->PathPoints.Num() > 1)
	{
		//return next point in the path
		return navPath->PathPoints[1];
	}

	//Failed to find path
	return GetActorLocation();
}

void AShooterTrackerBot::onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(!materialInstance)
	{
		materialInstance = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
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

	TArray<AActor*> ignoreDamageActors{ this };
	UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(), DamageRadius, DamageType, ignoreDamageActors, this, GetInstigatorController(), true);

	if (DebugTrackerBotDrawing > 0)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 12, FColor::Red, false, 2.f, 0, 2.f);
	}
	Destroy();
}

void AShooterTrackerBot::damageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

// Called every frame
void AShooterTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float distanceToTarget = (GetActorLocation() - nextPathPoint).Size();
	if(distanceToTarget <= RequiredDistanceToTarget)
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

void AShooterTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(!bStartedSelfDestruction)
	{
		if (AShooterCharacter* shooterChar = Cast<AShooterCharacter>(OtherActor))
		{
			//Start self destruction sequence
			GetWorldTimerManager().SetTimer(timerHandle_SelfDamage, this, &AShooterTrackerBot::damageSelf, 0.5, true, 0.0f);

			bStartedSelfDestruction = true;
		}
	}
}
