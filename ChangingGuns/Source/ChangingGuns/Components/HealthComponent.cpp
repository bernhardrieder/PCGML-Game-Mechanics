// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealNetwork.h"

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

void UHealthComponent::handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(Damage <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);

	if(GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s Health Changed: %s"), *GetOwner()->GetName(), *FString::SanitizeFloat(Health));
	}

	OnHealthChangedEvent.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
}
