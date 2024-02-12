// Fill out your copyright notice in the Description page of Project Settings.


#include "CHPlayerController.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/HUD/CHPlayerHUD.h"
#include "ContactHostile/HUD/PlayerOverlay.h"
#include "ContactHostile/HUD/Announcement.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "ContactHostile/GameMode/CHGameMode.h"
#include "ContactHostile/ContactHostileComponents/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ContactHostile/GameState/CHGameState.h"
#include <ContactHostile/PlayerState/CHPlayerState.h>


void ACHPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CHPlayerHUD = Cast<ACHPlayerHUD>(GetHUD());
	ServerCheckMatchState();
}

void ACHPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACHPlayerController, MatchState);
}


void ACHPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ACHPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void ACHPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncTimer += DeltaTime;
	if (TimeSyncTimer > TimeSyncFrequency)
	{
		if (IsLocalController())
		{
			ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		}
		TimeSyncTimer = 0.f;
	}
}

void ACHPlayerController::ServerCheckMatchState_Implementation()
{
	ACHGameMode* GameMode = Cast<ACHGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		StartingTime = GameMode->LevelStartingTime;

		// Replicated
		MatchState = GameMode->GetMatchState();

		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, StartingTime);

		//if (CHPlayerHUD && MatchState == MatchState::WaitingToStart && CHPlayerHUD->AnnouncementOverlay == nullptr)
		//{
		//	CHPlayerHUD->AddAnnouncement();
		//}
	}
}

void ACHPlayerController::ClientJoinMidGame_Implementation(FName LevelState, float LevelWarmupTime, float LevelMatchTime, float LevelCooldownTime, float LevelStartingTime)
{
	WarmupTime = LevelWarmupTime;
	MatchTime = LevelMatchTime;
	CooldownTime = LevelCooldownTime;
	StartingTime = LevelStartingTime;

	MatchState = LevelState;

	OnMatchStateSet(MatchState);

	if (CHPlayerHUD && MatchState == MatchState::WaitingToStart)
	{
		CHPlayerHUD->AddAnnouncement();
	}
}

void ACHPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) { TimeLeft = WarmupTime - GetServerTime() + StartingTime; }
	else if (MatchState == MatchState::InProgress) { TimeLeft = WarmupTime + MatchTime - GetServerTime() + StartingTime; }
	else if (MatchState == MatchState::Cooldown) { TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + StartingTime; }

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		CHGameMode = CHGameMode == nullptr ? Cast<ACHGameMode>(UGameplayStatics::GetGameMode(this)) : CHGameMode;
		if (CHGameMode)
		{
			SecondsLeft = FMath::CeilToInt(CHGameMode->GetCountdownTime() + StartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementTimer(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchTimer(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void ACHPlayerController::PollInit()
{
	if (CHPlayerOverlay == nullptr)
	{
		if (CHPlayerHUD && CHPlayerHUD->PlayerOverlay)
		{
			CHPlayerOverlay = CHPlayerHUD->PlayerOverlay;
			if (CHPlayerOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMakHealth);
				SetHUDScore(HUDScore);
				SetHUDKilled(HUDKilledCount);
			}
		}
	}
}

void ACHPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfRecipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfRecipt);
}

void ACHPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeOfServerRecievedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeOfServerRecievedClientRequest + (0.5f * RoundTripTime);

	ClientServerDeltaTime = CurrentServerTime - GetWorld()->GetTimeSeconds();
}


float ACHPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDeltaTime;
}

void ACHPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AContactHostileCharacter* CHCharacter = Cast<AContactHostileCharacter>(InPawn);
	if (CHCharacter)
	{
		SetHUDHealth(CHCharacter->GetHealth(), CHCharacter->GetMaxHealt());
	}
}

void ACHPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD && 
		CHPlayerHUD->PlayerOverlay && 
		CHPlayerHUD->PlayerOverlay->HealthBar && 
		CHPlayerHUD->PlayerOverlay->HealthText;

	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		CHPlayerHUD->PlayerOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		CHPlayerHUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
	} else {
		bInitializeCHPlayerOverlay = true;
		HUDHealth = Health;
		HUDMakHealth = MaxHealth;
	}

}

void ACHPlayerController::SetHUDScore(float Score)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->PlayerOverlay &&
		CHPlayerHUD->PlayerOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		CHPlayerHUD->PlayerOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	} else {
		bInitializeCHPlayerOverlay = true;
		HUDScore = Score;
	}
}

void ACHPlayerController::SetHUDKilled(int32 KilledCount)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->PlayerOverlay &&
		CHPlayerHUD->PlayerOverlay->KilledAmount;

	if (bHUDValid)
	{
		FString KilledText = FString::Printf(TEXT("%d"), KilledCount);
		CHPlayerHUD->PlayerOverlay->KilledAmount->SetText(FText::FromString(KilledText));
	} else {
		bInitializeCHPlayerOverlay = true;
		HUDKilledCount = KilledCount;
	}
}

void ACHPlayerController::SetHUDWeaponAmmo(int32 AmmoCount)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->PlayerOverlay &&
		CHPlayerHUD->PlayerOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), AmmoCount);
		CHPlayerHUD->PlayerOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ACHPlayerController::SetHUDCarriedAmmo(int32 AmmoCount)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->PlayerOverlay &&
		CHPlayerHUD->PlayerOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), AmmoCount);
		CHPlayerHUD->PlayerOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ACHPlayerController::SetHUDMatchTimer(float Time)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->PlayerOverlay &&
		CHPlayerHUD->PlayerOverlay->MatchTimerText;

	if (bHUDValid)
	{
		if (Time < 0.f)
		{
			CHPlayerHUD->PlayerOverlay->MatchTimerText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;
		FString TimerText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CHPlayerHUD->PlayerOverlay->MatchTimerText->SetText(FText::FromString(TimerText));
	}
}

void ACHPlayerController::SetHUDAnnouncementTimer(float Time)
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	bool bHUDValid = CHPlayerHUD &&
		CHPlayerHUD->AnnouncementOverlay &&
		CHPlayerHUD->AnnouncementOverlay->WarmupTimeText;

	if (bHUDValid)
	{
		if (Time < 0.f) 
		{
			CHPlayerHUD->AnnouncementOverlay->WarmupTimeText->SetText(FText());
			return; 
		}

		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;
		FString TimerText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CHPlayerHUD->AnnouncementOverlay->WarmupTimeText->SetText(FText::FromString(TimerText));
	}
}

void ACHPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
}

void ACHPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleMatchCooldown();
	}
}

void ACHPlayerController::HandleMatchHasStarted()
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	if (CHPlayerHUD)
	{
		CHPlayerHUD->AddCharacterOverlay();
		if (CHPlayerHUD->AnnouncementOverlay)
		{
			CHPlayerHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACHPlayerController::HandleMatchCooldown()
{
	CHPlayerHUD = CHPlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : CHPlayerHUD;
	if (CHPlayerHUD)
	{
		CHPlayerHUD->PlayerOverlay->RemoveFromParent();

		bool bHudValid = CHPlayerHUD->AnnouncementOverlay &&
			CHPlayerHUD->AnnouncementOverlay->AnnouncementText &&
			CHPlayerHUD->AnnouncementOverlay->InfoText;

		if (bHudValid)
		{
			FString AnnouncementText("New Match Starts In:");
			CHPlayerHUD->AnnouncementOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ACHGameState* CHGameState = Cast<ACHGameState>(UGameplayStatics::GetGameState(this));
			ACHPlayerState* CHPlayerState = GetPlayerState<ACHPlayerState>();

			FString InfoTextString("");
			if (CHGameState && CHPlayerState)
			{
				TArray<ACHPlayerState*> TopPlayers = CHGameState->TopScoringPlayers;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("No winners");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == CHPlayerState)
				{
					InfoTextString = FString("You win!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner:\n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Tied:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
			}

			CHPlayerHUD->AnnouncementOverlay->InfoText->SetText(FText::FromString(InfoTextString));
			CHPlayerHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
	AContactHostileCharacter* CHCharacter = Cast<AContactHostileCharacter>(GetPawn());
	if (CHCharacter)
	{
		CHCharacter->bDisableGameplay = true;
		if (CHCharacter->GetCombat())
		{
			CHCharacter->GetCombat()->FireButtonPressed(false);
			CHCharacter->GetCombat()->SetAiming(false);
		}
	}
}
