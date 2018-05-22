// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Health, float, HealthDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

class UDamageType;
class AController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THESISPROTOTYPE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	uint8 TeamNumber;

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

	FORCEINLINE float GetHealth() const { return Health; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//this function triggers health change on the client
	UFUNCTION()
	void OnRep_Health(float oldHealth); // can have the old value as parameter!

	UFUNCTION()
	void handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:
	UPROPERTY(BlueprintAssignable, Category = "Health Component")
	FOnHealthChangedEvent OnHealthChangedEvent;

	UFUNCTION(BlueprintPure, Category = "Health Component")
	static bool IsFriendly(AActor* actorA, AActor* actorB);

protected:
	bool bIsDead = false;
};
