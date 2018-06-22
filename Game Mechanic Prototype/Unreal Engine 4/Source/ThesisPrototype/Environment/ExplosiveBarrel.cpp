// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ExplosiveBarrel.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "ChangingGuns.h"

// Sets default values
AExplosiveBarrel::AExplosiveBarrel()
{
	meshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	meshComp->SetSimulatePhysics(true);
	meshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = meshComp;

	healthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	healthComp->SetTeamNumber(TEAMNUMBER_ENVIRONMENT);

	radialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	radialForceComp->SetupAttachment(RootComponent);
	radialForceComp->bImpulseVelChange = true;
	radialForceComp->bAutoActivate = false;
	radialForceComp->bIgnoreOwningActor = true;

	explosionImpulse = 400;
	baseDamage = 150;
	damageRadius = 250.f;
}

void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	healthComp->OnHealthChangedEvent.AddDynamic(this, &AExplosiveBarrel::onHealthChanged);
	radialForceComp->Radius = damageRadius;
}

void AExplosiveBarrel::onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(bExploded)
	{
		return;
	}

	if(Health <= 0.f)
	{
		bExploded = true;
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), explosionEffect, GetActorLocation());
		meshComp->SetMaterial(0, explodeMaterial);
		UGameplayStatics::PlaySoundAtLocation(this, explosionSound, GetActorLocation());

		TArray<AActor*> ignoreDamageActors{ this };
		UGameplayStatics::ApplyRadialDamage(GetWorld(), baseDamage, GetActorLocation(), damageRadius, damageType, ignoreDamageActors, this, GetInstigatorController(), true);

		FVector boostIntensity = FVector::UpVector * explosionImpulse;
		meshComp->AddImpulse(boostIntensity, NAME_None, true);

		radialForceComp->FireImpulse();
	}
}
