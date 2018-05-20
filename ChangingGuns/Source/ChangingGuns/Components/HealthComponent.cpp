// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealNetwork.h"

static int32 DebugHealthComponents = 1;
FAutoConsoleVariableRef CVARDebuHealthComponents(
	TEXT("Game.DebugHealth"),
	DebugHealthComponents,
	TEXT("Output Debug for Health Components"),
	ECVF_Cheat
);


void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
}

UHealthComponent::UHealthComponent()
{
	DefaultHealth = 100;

	SetIsReplicated(true);
}

void UHealthComponent::Heal(float HealAmount)
{
	if(HealAmount < 0.f || Health <= 0.f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.f, DefaultHealth);

	if (DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (+%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount));
	}

	OnHealthChangedEvent.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	//only hook if we are server
	if(GetOwnerRole() == ROLE_Authority)
	{
		if (AActor* owner = GetOwner())
		{
			owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::handleTakeAnyDamage);
		}
	}
	Health = DefaultHealth;
}

void UHealthComponent::OnRep_Health(float oldHealth)
{
	float damage = Health - oldHealth;
	OnHealthChangedEvent.Broadcast(this, Health, damage, nullptr, nullptr, nullptr);
}

void UHealthComponent::handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(Damage <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);

	if(DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(Damage));
	}

	OnHealthChangedEvent.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
}
