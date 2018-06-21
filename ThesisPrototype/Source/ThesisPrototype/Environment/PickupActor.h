// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class USphereComponent;
class UDecalComponent;
class APowerUpActor;

UCLASS()
class THESISPROTOTYPE_API APickupActor : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* sphereComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UDecalComponent* decalComp;

	UPROPERTY(EditInstanceOnly, Category = "Pickup Actor")
	TSubclassOf<APowerUpActor> powerUpClass;

	UPROPERTY(EditInstanceOnly, Category = "Pickup Actor")
	float coolDownDuration;

public:
	APickupActor();
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;
	void respawnPowerUp();

protected:
	APowerUpActor* spawnedPowerUp;
	FTimerHandle timerHandle_RespawnTimer;
};
