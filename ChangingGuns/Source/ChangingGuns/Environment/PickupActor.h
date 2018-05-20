// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class USphereComponent;
class UDecalComponent;
class APowerUpActor;

UCLASS()
class CHANGINGGUNS_API APickupActor : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereComp;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UDecalComponent* DecalComp;
	UPROPERTY(EditInstanceOnly, Category = "Pickup Actor")
	TSubclassOf<APowerUpActor> PowerUpClass;
	UPROPERTY(EditDefaultsOnly, Category = "Pickup Actor")
	float CoolDownDuration;
public:	
	APickupActor();

protected:
	virtual void BeginPlay() override;
	void respawnPowerUp();

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	APowerUpActor* spawnedPowerUp;
	FTimerHandle timerHandle_RespawnTimer;
};
