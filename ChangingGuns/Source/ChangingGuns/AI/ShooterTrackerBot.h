// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShooterTrackerBot.generated.h"

UCLASS()
class CHANGINGGUNS_API AShooterTrackerBot : public APawn
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	bool bUseVelocityChange;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
	float RequiredDistanceToTarget;
public:
	// Sets default values for this pawn's properties
	AShooterTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector GetNextPathPoint();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	FVector nextPathPoint;
};
