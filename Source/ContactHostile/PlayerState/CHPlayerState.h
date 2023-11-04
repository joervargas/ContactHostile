// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CHPlayerState.generated.h"


class AContactHostileCharacter;
class ACHPlayerController;

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API ACHPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	void AddToScore(float Amount);
	void AddToKilledCount(int32 Amount);

	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_KilledCount();

private:

	UPROPERTY()
	AContactHostileCharacter* Character;
	
	UPROPERTY()
	ACHPlayerController* PlayerController;

	UPROPERTY(ReplicatedUsing = OnRep_KilledCount)
	int32 KilledCount;
};
