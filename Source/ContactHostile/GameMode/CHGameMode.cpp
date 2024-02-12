// Fill out your copyright notice in the Description page of Project Settings.


#include "CHGameMode.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/PlayerController/CHPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ContactHostile/PlayerState/CHPlayerState.h"
#include "ContactHostile/GameState/CHGameState.h"


namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ACHGameMode::ACHGameMode()
{
	bDelayedStart = true;
}

void ACHGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ACHGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - (GetWorld()->GetTimeSeconds() + LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = (WarmupTime + MatchTime) - (GetWorld()->GetTimeSeconds() + LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = (CooldownTime + WarmupTime + MatchTime) - (GetWorld()->GetTimeSeconds() + LevelStartingTime);
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}


void ACHGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// Loop through all player controllers on the server
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ACHPlayerController* CHPlayerController = Cast<ACHPlayerController>(*It);
		if (CHPlayerController)
		{
			CHPlayerController->OnMatchStateSet(MatchState);
		}
	}
}

void ACHGameMode::PlayerEliminated(AContactHostileCharacter* EliminatedCharacter, ACHPlayerController* EliminatedController, ACHPlayerController* AttackerController)
{
	if (EliminatedController == nullptr || AttackerController == nullptr) return;
	ACHPlayerState* AttackerPlayerState = AttackerController ? Cast<ACHPlayerState>(AttackerController->PlayerState) : nullptr;
	ACHPlayerState* EliminatedPlayerState = EliminatedController ? Cast<ACHPlayerState>(EliminatedController->PlayerState) : nullptr;

	ACHGameState* CHGameState = GetGameState<ACHGameState>();

	if (AttackerPlayerState && AttackerPlayerState != EliminatedPlayerState && CHGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		CHGameState->UpdateTopScore(AttackerPlayerState);

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

