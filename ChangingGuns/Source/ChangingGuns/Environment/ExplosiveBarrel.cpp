// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "ExplosiveBarrel.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values
AExplosiveBarrel::AExplosiveBarrel()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	ExplosionImpulse = 400;
	BaseDamage = 150;
}

void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	HealthComp->OnHealthChangedEvent.AddDynamic(this, &AExplosiveBarrel::onHealthChanged);
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

		FVector boostIntensity = FVector::UpVector * ExplosionImpulse;
		MeshComp->AddImpulse(boostIntensity, NAME_None, true);

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
		MeshComp->SetMaterial(0, ExplodeMaterial);

		RadialForceComp->FireImpulse();

		TArray<AActor*> ignoreDamageActors{ this };
		UGameplayStatics::ApplyRadialDamage(GetWorld(), BaseDamage, GetActorLocation(), RadialForceComp->Radius, DamageType, ignoreDamageActors);

	}
}
