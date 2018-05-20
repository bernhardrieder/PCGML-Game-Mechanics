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

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(75.f);
	RootComponent = SphereComp;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetRelativeRotation(FRotator(90, 0, 0));
	DecalComp->DecalSize = FVector(64.f, 75.f, 75.f);

	DecalComp->SetupAttachment(RootComponent);

	CoolDownDuration = 10.f;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		respawnPowerUp();
	}
}

void APickupActor::respawnPowerUp()
{
	if(!PowerUpClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("No PowerUp class specified in %s!"), *GetName());
		return;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	spawnedPowerUp = GetWorld()->SpawnActor<APowerUpActor>(PowerUpClass, GetTransform(), spawnParams);
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if(HasAuthority() && spawnedPowerUp)
	{
		spawnedPowerUp->ActivatePowerUp(OtherActor);
		spawnedPowerUp = nullptr;

		GetWorldTimerManager().SetTimer(timerHandle_RespawnTimer, this, &APickupActor::respawnPowerUp, CoolDownDuration);
	}
}

