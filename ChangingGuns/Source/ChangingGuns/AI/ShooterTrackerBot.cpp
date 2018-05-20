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

	MovementForce = 1000.f;
	bUseVelocityChange = false;
	RequiredDistanceToTarget = 100.f;
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
	}
}
