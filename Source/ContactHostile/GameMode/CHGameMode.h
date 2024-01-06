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

	ACHGameMode();

	virtual void Tick(float DeltaTime) override;
	
	virtual void PlayerEliminated(AContactHostileCharacter* EliminatedCharacter, ACHPlayerController* EliminatedController, ACHPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedCharController);

public:

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 5.f;

	float LevelStartingTime = 0.f;

protected:

	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

private:

	float CountDownTime = 0.f;
};
