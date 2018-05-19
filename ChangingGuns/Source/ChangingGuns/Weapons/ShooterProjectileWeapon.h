// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "ShooterProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class CHANGINGGUNS_API AShooterProjectileWeapon : public AShooterWeapon
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> ProjectileClass;
public:

	void Fire() override;
	
	
};
