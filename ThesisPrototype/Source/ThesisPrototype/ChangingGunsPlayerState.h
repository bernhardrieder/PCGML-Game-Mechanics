// Copyright 2018 - Bernhard Rieder - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ChangingGunsPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class THESISPROTOTYPE_API AChangingGunsPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Player State")
	void AddScore(float ScoreDelta);
	
	
};
