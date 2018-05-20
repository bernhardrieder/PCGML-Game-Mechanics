// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedEvent, const class UHealthComponent*, HealthComp, float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

class UDamageType;
class AController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHANGINGGUNS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Health Component")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float DefaultHealth;

public:	
	// Sets default values for this component's properties
	UHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedEvent OnHealthChangedEvent;
};
