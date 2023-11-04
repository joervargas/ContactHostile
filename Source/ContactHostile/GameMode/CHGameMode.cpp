// Fill out your copyright notice in the Description page of Project Settings.


#include "CHGameMode.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/PlayerController/CHPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ContactHostile/PlayerState/CHPlayerState.h"


void ACHGameMode::PlayerEliminated(AContactHostileCharacter* EliminatedCharacter, ACHPlayerController* EliminatedController, ACHPlayerController* AttackerController)
{
	ACHPlayerState* AttackerPlayerState = AttackerController ? Cast<ACHPlayerState>(AttackerController->PlayerState) : nullptr;
	ACHPlayerState* EliminatedPlayerState = EliminatedController ? Cast<ACHPlayerState>(EliminatedController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != EliminatedPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
		EliminatedPlayerState->AddToKilledCount(1);
	}
	//if (EliminatedPlayerState && EliminatedPlayerState != AttackerPlayerState)
	//{
	//	EliminatedPlayerState->AddToKilledCount(1);
	//}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}

void ACHGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedCharController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}
	if (EliminatedCharController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedCharController, PlayerStarts[Selection]);
	}
}
