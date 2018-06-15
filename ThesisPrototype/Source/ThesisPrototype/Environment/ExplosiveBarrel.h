// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplosiveBarrel.generated.h"

class UHealthComponent;
class URadialForceComponent;
class UParticleSystem;
class UMaterialInterface;
class UDamageType;
class USoundCue;

UCLASS()
class THESISPROTOTYPE_API AExplosiveBarrel : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UHealthComponent* healthComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* meshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URadialForceComponent* radialForceComp;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	float explosionImpulse;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* explosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UMaterialInterface* explodeMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	float baseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	TSubclassOf<UDamageType> damageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	float damageRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* explosionSound;

public:
	AExplosiveBarrel();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser);

protected:
	bool bExploded = false;
};
