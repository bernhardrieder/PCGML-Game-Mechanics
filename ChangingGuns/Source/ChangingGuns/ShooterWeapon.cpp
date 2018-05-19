// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AShooterWeapon::AShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";
}

// Called when the game starts or when spawned
void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AShooterWeapon::Fire()
{
	//trace the world, from pawn eyes to crosshair location

	if(AActor* owner = GetOwner())
	{
		FVector eyeLocation;
		FRotator eyeRotator;
		owner->GetActorEyesViewPoint(eyeLocation, eyeRotator);

		FVector shootDirection = eyeRotator.Vector();
		FVector traceEnd = eyeLocation + shootDirection * 10000;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(owner);
		queryParams.AddIgnoredActor(this);
		queryParams.bTraceComplex = true; //gives us the exact result because traces every triangle instead of a simple collider

		FHitResult hitResult;
		if(GetWorld()->LineTraceSingleByChannel(hitResult, eyeLocation, traceEnd, ECC_Visibility, queryParams))
		{
			//is blocking hit! -> process damage
			AActor* hitActor = hitResult.GetActor();
			UGameplayStatics::ApplyPointDamage(hitActor, 20.f, shootDirection, hitResult, owner->GetInstigatorController(), this, DamageType);

			if (ImpactEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, hitResult.ImpactPoint, hitResult.ImpactNormal.Rotation());
			}

		}
		DrawDebugLine(GetWorld(), eyeLocation, traceEnd, FColor::White, false, 1.f, 0, 1.f);

		if(MuzzleEffect)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		}

		if(TracerEffect)
		{
			FVector muzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
			UParticleSystemComponent* particleSystem = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, muzzleLocation);
			FVector tracerEndPoint = hitResult.IsValidBlockingHit() ? hitResult.ImpactPoint : traceEnd;
			particleSystem->SetVectorParameter(TracerTargetName, tracerEndPoint);
		}
	}

}

// Called every frame
void AShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

