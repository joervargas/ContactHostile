// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "CHGameState.generated.h"

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API ACHGameState : public AGameState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(class ACHPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<ACHPlayerState*> TopScoringPlayers;

private:

	float TopScore = 0.f;
	
};
