// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealNetwork.h"
#include "ChangingGunsGameMode.h"
#include "Engine/World.h"

static int32 DebugHealthComponents = 1;
FAutoConsoleVariableRef CVARDebuHealthComponents(
	TEXT("Game.DebugHealth"),
	DebugHealthComponents,
	TEXT("Output Debug for Health Components"),
	ECVF_Cheat
);

UHealthComponent::UHealthComponent()
{
	DefaultHealth = 100;
	TeamNumber = 255;
	DefaultArmor = 0;
	DefaultExtraLives = 0;
	bHandleDamageEnabled = true;
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

void UHealthComponent::RepairArmor(float RepairAmount)
{
	if (RepairAmount <= 0.f)
	{
		return;
	}
	Armor = FMath::Clamp(Armor + RepairAmount, 0.f, DefaultArmor);

	if (DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Armor changed to %s (+%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Armor), *FString::SanitizeFloat(RepairAmount));
	}

	OnArmorChangedEvent.Broadcast(this, Armor, -RepairAmount, nullptr, nullptr, nullptr);
}

void UHealthComponent::RestoreExtraLife(int32 amount)
{
	ExtraLives = FMath::Clamp(ExtraLives + amount, 0, DefaultExtraLives);

	OnExtraLivesChangedEvent.Broadcast(this, ExtraLives);
}

void UHealthComponent::SetHandleDamageEnabled(bool val)
{
	bHandleDamageEnabled = val;
}

void UHealthComponent::SetTeamNumber(uint8 teamNumber)
{
	TeamNumber = teamNumber;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	//only hook if we are server
	if (AActor* owner = GetOwner())
	{
		owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::handleTakeAnyDamage);
	}
	Health = DefaultHealth;
	Armor = DefaultArmor;
	ExtraLives = DefaultExtraLives;
}

void UHealthComponent::handleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if(Damage <= 0.0f || bIsDead || !bHandleDamageEnabled)
	{
		return;
	}

	if(DamagedActor != DamageCauser && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	if(Armor > 0.f)
	{
		Armor = FMath::Clamp(Armor - Damage, 0.f, DefaultArmor);
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Armor changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Armor), *FString::SanitizeFloat(Damage));
		}
		OnArmorChangedEvent.Broadcast(this, Armor, Damage, nullptr, nullptr, nullptr);
		return;
	}
	Health = FMath::Clamp(Health - Damage, 0.f, DefaultHealth);

	if(Health <= 0.f && ExtraLives > 0)
	{
		--ExtraLives;
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Lifes changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(ExtraLives), *FString::SanitizeFloat(-1));
		}
		OnExtraLivesChangedEvent.Broadcast(this, ExtraLives);
		Health = DefaultHealth;
		OnHealthChangedEvent.Broadcast(this, Health, DefaultHealth, DamageType, InstigatedBy, DamageCauser);
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(DefaultHealth));
		}
		return;
	}
	bIsDead = Health <= 0.f;

	if(DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(Damage));
	}

	OnHealthChangedEvent.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if(bIsDead)
	{
		if (AChangingGunsGameMode* gm = Cast<AChangingGunsGameMode>(GetOwner()->GetWorld()->GetAuthGameMode()))
		{
			gm->OnActorKilledEvent.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

bool UHealthComponent::IsFriendly(AActor* actorA, AActor* actorB)
{
	if (!actorA || !actorB)
	{
		return true;
	}

	UHealthComponent* healthCompA = Cast<UHealthComponent>(actorA->GetComponentByClass(StaticClass()));
	UHealthComponent* healthCompB = Cast<UHealthComponent>(actorB->GetComponentByClass(StaticClass()));

	if(!healthCompA || !healthCompB)
	{
		return true;
	}

	return healthCompA->TeamNumber == healthCompB->TeamNumber;
}
