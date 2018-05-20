// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShooterTrackerBot.generated.h"

class UHealthComponent;
class USphereComponent;

UCLASS()
class CHANGINGGUNS_API AShooterTrackerBot : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Player")
	UHealthComponent* HealthComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Player")
	USphereComponent* SphereComp;
	
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float RequiredDistanceToTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracker Bot")
	float DamageRadius;

	UPROPERTY()
	bool bExploded = false;

public:
	// Sets default values for this pawn's properties
	AShooterTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector GetNextPathPoint();

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser);

	void selfDestruct();
	void damageSelf();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	FVector nextPathPoint;
	UMaterialInstanceDynamic* materialInstance = nullptr;
	FTimerHandle timerHandle_SelfDamage;
	bool bStartedSelfDestruction = false;
};
