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
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->SetTeamNumber(TEAMNUMBER_ENVIRONMENT);

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	ExplosionImpulse = 400;
	BaseDamage = 150;
	DamageRadius = 250.f;


}

void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	HealthComp->OnHealthChangedEvent.AddDynamic(this, &AExplosiveBarrel::onHealthChanged);
	RadialForceComp->Radius = DamageRadius;
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
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
		MeshComp->SetMaterial(0, ExplodeMaterial);
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());

		TArray<AActor*> ignoreDamageActors{ this };
		UGameplayStatics::ApplyRadialDamage(GetWorld(), BaseDamage, GetActorLocation(), DamageRadius, DamageType, ignoreDamageActors, this, GetInstigatorController(), true);

		FVector boostIntensity = FVector::UpVector * ExplosionImpulse;
		MeshComp->AddImpulse(boostIntensity, NAME_None, true);

		RadialForceComp->FireImpulse();
	}
}
