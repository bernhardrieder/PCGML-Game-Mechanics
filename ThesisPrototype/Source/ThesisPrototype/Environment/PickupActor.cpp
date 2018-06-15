// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "PickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "PowerUpActor.h"
#include "TimerManager.h"

// Sets default values
APickupActor::APickupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	sphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	sphereComp->SetSphereRadius(75.f);
	RootComponent = sphereComp;

	decalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	decalComp->SetRelativeRotation(FRotator(90, 0, 0));
	decalComp->DecalSize = FVector(64.f, 75.f, 75.f);

	decalComp->SetupAttachment(RootComponent);

	coolDownDuration = 10.f;
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();

	respawnPowerUp();
}

void APickupActor::respawnPowerUp()
{
	if(!powerUpClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("No PowerUp class specified in %s!"), *GetName());
		return;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	spawnedPowerUp = GetWorld()->SpawnActor<APowerUpActor>(powerUpClass, GetTransform(), spawnParams);
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(spawnedPowerUp && OtherActor)
	{
		APawn* pawn = Cast<APawn>(OtherActor);
		if(!pawn || !pawn->IsPlayerControlled())
		{
			return;
		}
		spawnedPowerUp->ActivatePowerUp(OtherActor);
		spawnedPowerUp = nullptr;

		GetWorldTimerManager().SetTimer(timerHandle_RespawnTimer, this, &APickupActor::respawnPowerUp, coolDownDuration);
	}
}

