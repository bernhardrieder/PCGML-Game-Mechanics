// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "UnrealNetwork.h"
#include "ChangingGunsGameMode.h"
#include "Engine/World.h"
#include "ChangingGuns.h"

static int32 DebugHealthComponents = 0;
FAutoConsoleVariableRef CVARDebuHealthComponents(
	TEXT("Game.DebugHealth"),
	DebugHealthComponents,
	TEXT("Output Debug for Health Components"),
	ECVF_Cheat
);

UHealthComponent::UHealthComponent()
{
	defaultHealth = 100;
	teamNumber = 255;
	defaultArmor = 0;
	defaultExtraLives = 0;
	bHandleDamageEnabled = true;
}

void UHealthComponent::Heal(float HealAmount)
{
	if(HealAmount < 0.f || health <= 0.f)
	{
		return;
	}

	health = FMath::Clamp(health + HealAmount, 0.f, defaultHealth);

	if (DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (+%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(health), *FString::SanitizeFloat(HealAmount));
	}

	OnHealthChangedEvent.Broadcast(this, health, -HealAmount, nullptr, nullptr, nullptr);
}

void UHealthComponent::RepairArmor(float RepairAmount)
{
	if (RepairAmount <= 0.f)
	{
		return;
	}
	armor = FMath::Clamp(armor + RepairAmount, 0.f, defaultArmor);

	if (DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Armor changed to %s (+%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(armor), *FString::SanitizeFloat(RepairAmount));
	}

	OnArmorChangedEvent.Broadcast(this, armor, -RepairAmount, nullptr, nullptr, nullptr);
}

void UHealthComponent::RestoreExtraLife(int32 Lives)
{
	extraLives = FMath::Clamp(extraLives + Lives, 0, defaultExtraLives);

	OnExtraLivesChangedEvent.Broadcast(this, extraLives);
}

void UHealthComponent::SetHandleDamageEnabled(bool HandleDamage)
{
	bHandleDamageEnabled = HandleDamage;
}

void UHealthComponent::SetTeamNumber(uint8 TeamNumber)
{
	teamNumber = TeamNumber;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	//only hook if we are server
	if (AActor* owner = GetOwner())
	{
		owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::handleTakeAnyDamage);
	}
	health = defaultHealth;
	armor = defaultArmor;
	extraLives = defaultExtraLives;
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

	if(armor > 0.f)
	{
		armor = FMath::Clamp(armor - Damage, 0.f, defaultArmor);
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Armor changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(armor), *FString::SanitizeFloat(Damage));
		}
		OnArmorChangedEvent.Broadcast(this, armor, Damage, nullptr, nullptr, nullptr);
		return;
	}
	health = FMath::Clamp(health - Damage, 0.f, defaultHealth);

	if(health <= 0.f && extraLives > 0)
	{
		--extraLives;
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Lifes changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(extraLives), *FString::SanitizeFloat(-1));
		}
		OnExtraLivesChangedEvent.Broadcast(this, extraLives);
		health = defaultHealth;
		OnHealthChangedEvent.Broadcast(this, health, defaultHealth, DamageType, InstigatedBy, DamageCauser);
		if (DebugHealthComponents > 0 && GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(health), *FString::SanitizeFloat(defaultHealth));
		}
		return;
	}
	bIsDead = health <= 0.f;

	if(DebugHealthComponents > 0 && GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("%s - Health changed to %s (-%s)"), *GetOwner()->GetName(), *FString::SanitizeFloat(health), *FString::SanitizeFloat(Damage));
	}

	OnHealthChangedEvent.Broadcast(this, health, Damage, DamageType, InstigatedBy, DamageCauser);

	if(bIsDead)
	{
		if (AChangingGunsGameMode* gm = Cast<AChangingGunsGameMode>(GetOwner()->GetWorld()->GetAuthGameMode()))
		{
			gm->OnActorKilledEvent.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

bool UHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (!ActorA || !ActorB)
	{
		return true;
	}

	UHealthComponent* healthCompA = Cast<UHealthComponent>(ActorA->GetComponentByClass(StaticClass()));
	UHealthComponent* healthCompB = Cast<UHealthComponent>(ActorB->GetComponentByClass(StaticClass()));

	if(!healthCompA || !healthCompB)
	{
		return true;
	}
	
	return healthCompA->teamNumber == healthCompB->teamNumber || IsBot(ActorA) == IsBot(ActorB);
}


bool UHealthComponent::IsBot(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	UHealthComponent* healthComp = Cast<UHealthComponent>(Actor->GetComponentByClass(StaticClass()));

	if (!healthComp)
	{
		return false;
	}

	return healthComp->teamNumber >= TEAMNUMBER_BOT_RANGE_MIN && healthComp->teamNumber <= TEAMNUMBER_BOT_RANGE_MAX;
}