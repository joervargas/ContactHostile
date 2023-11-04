// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CHGameMode.generated.h"

class AContactHostileCharacter;
class ACHPlayerController;
/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API ACHGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	
	virtual void PlayerEliminated(AContactHostileCharacter* EliminatedCharacter, ACHPlayerController* EliminatedController, ACHPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedCharController);
};
