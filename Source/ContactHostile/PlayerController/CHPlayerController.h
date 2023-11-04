// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CHPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API ACHPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

private:

	class ACHPlayerHUD* PlayerHUD = nullptr;

public:

	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDHealth(float Health, float MaxHealth);

	void SetHUDScore(float Score);
	void SetHUDKilled(int32 KilledCount);
};
