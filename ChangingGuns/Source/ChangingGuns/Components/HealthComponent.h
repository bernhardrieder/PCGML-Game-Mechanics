// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Health, float, HealthDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

class UDamageType;
class AController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHANGINGGUNS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health Component")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float DefaultHealth;

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void Heal(float HealAmount);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//this function triggers health change on the client
	UFUNCTION()
	void OnRep_Health(float oldHealth); // can have the old value as parameter!

	UFUNCTION()
	void handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedEvent OnHealthChangedEvent;
};
