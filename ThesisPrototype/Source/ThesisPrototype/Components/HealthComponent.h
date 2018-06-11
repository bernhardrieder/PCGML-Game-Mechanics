// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Health, float, HealthDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnArmorChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Armor, float, ArmorDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExtraLivesChangedEvent, const class UHealthComponent*, OwningHealthComp, int32, extraLives);

class UDamageType;
class AController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THESISPROTOTYPE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	uint8 TeamNumber;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float DefaultHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	float Armor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float DefaultArmor;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	int32 ExtraLives;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	int32 DefaultExtraLives;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	bool bHandleDamageEnabled;

public:
	// Sets default values for this component's properties
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void RepairArmor(float RepairAmount);

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void RestoreExtraLife(int32 amount);


	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetArmor() const { return Armor; }

	FORCEINLINE bool IsHandlingDamage() const { return bHandleDamageEnabled; }

	UFUNCTION(BlueprintCallable, Category="Health Component")
	void SetHandleDamageEnabled(bool val);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:
	UPROPERTY(BlueprintAssignable, Category = "Health Component")
	FOnHealthChangedEvent OnHealthChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Health Component")
	FOnArmorChangedEvent OnArmorChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Health Component")
	FOnExtraLivesChangedEvent OnExtraLivesChangedEvent;

	UFUNCTION(BlueprintPure, Category = "Health Component")
	static bool IsFriendly(AActor* actorA, AActor* actorB);

protected:
	bool bIsDead = false;
};
