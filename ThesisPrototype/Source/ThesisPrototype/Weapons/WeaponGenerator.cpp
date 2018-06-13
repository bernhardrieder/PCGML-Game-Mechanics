// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "WeaponGenerator.h"


// Sets default values
AWeaponGenerator::AWeaponGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWeaponGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

