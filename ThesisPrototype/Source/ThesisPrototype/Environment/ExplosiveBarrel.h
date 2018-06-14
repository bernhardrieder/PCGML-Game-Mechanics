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
	UHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URadialForceComponent* RadialForceComp;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	float ExplosionImpulse;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UMaterialInterface* ExplodeMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosive Barrel")
	float DamageRadius;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SFX")
	USoundCue* ExplosionSound;

public:
	AExplosiveBarrel();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void onHealthChanged(const UHealthComponent* HealthComponent, float Health, float HealthDelta, const UDamageType* healthDamageType, AController* InstigatedBy, AActor* DamageCauser);

protected:
	bool bExploded = false;
};
