// Fill out your copyright notice in the Description page of Project Settings.


#include "CHPlayerState.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/PlayerController/CHPlayerController.h"
#include "Net/UnrealNetwork.h"


void ACHPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACHPlayerState, KilledCount, COND_OwnerOnly);
}

void ACHPlayerState::AddToScore(float Amount)
{
	SetScore(GetScore() + Amount);

	Character = Character == nullptr ? Cast<AContactHostileCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = (PlayerController == nullptr) ? Cast<ACHPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetHUDScore(GetScore());
		}
	}
}

void ACHPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = (Character == nullptr) ? Cast<AContactHostileCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = (PlayerController == nullptr) ? Cast<ACHPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetHUDScore(GetScore());
		}
	}
}

void ACHPlayerState::AddToKilledCount(int32 Amount)
{
	KilledCount += Amount;

	Character = Character == nullptr ? Cast<AContactHostileCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = (PlayerController == nullptr) ? Cast<ACHPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetHUDKilled(KilledCount);
		}
	}
}

void ACHPlayerState::OnRep_KilledCount()
{
	Character = Character == nullptr ? Cast<AContactHostileCharacter>(GetPawn()) : Character;
	if (Character)
	{
		PlayerController = (PlayerController == nullptr) ? Cast<ACHPlayerController>(Character->Controller) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetHUDKilled(KilledCount);
		}
	}
}

