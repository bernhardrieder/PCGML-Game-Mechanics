// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Health, float, HealthDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnArmorChangedEvent, const class UHealthComponent*, OwningHealthComp, float, Armor, float, ArmorDelta, const class UDamageType*, CausedDamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExtraLivesChangedEvent, const class UHealthComponent*, OwningHealthComp, int32, ExtraLives);

class UDamageType;
class AController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THESISPROTOTYPE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	uint8 teamNumber;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	float health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float defaultHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	float armor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	float defaultArmor;

	UPROPERTY(BlueprintReadOnly, Category = "Health Component")
	int32 extraLives;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health Component")
	int32 defaultExtraLives;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	bool bHandleDamageEnabled;

public:
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void RepairArmor(float RepairAmount);

	UFUNCTION(BlueprintCallable, Category = "Health Component")
	void RestoreExtraLife(int32 Lives);


	FORCEINLINE uint8 GetTeamNumber() const { return teamNumber; }
	FORCEINLINE float GetHealth() const { return health; }
	FORCEINLINE float GetArmor() const { return armor; }
	FORCEINLINE bool IsHandlingDamage() const { return bHandleDamageEnabled; }

	UFUNCTION(BlueprintCallable, Category="Health Component")
	void SetHandleDamageEnabled(bool HandleDamage);

	void SetTeamNumber(uint8 TeamNumber);

	UFUNCTION(BlueprintPure, Category = "Health Component")
	static bool IsFriendly(AActor* ActorA, AActor*ActorB);

	UFUNCTION(BlueprintPure, Category = "Health Component")
	static bool IsBot(AActor* Actor);

protected:
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

protected:
	bool bIsDead = false;
};
