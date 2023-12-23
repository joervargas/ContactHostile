// Fill out your copyright notice in the Description page of Project Settings.


#include "CHPlayerController.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/HUD/CHPlayerHUD.h"
#include "ContactHostile/HUD/PlayerOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"


void ACHPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CHPlayerHUD = Cast<ACHPlayerHUD>(GetHUD());
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

void ACHPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchTimer(SecondsLeft);
		CountdownInt = SecondsLeft;
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
	//float ServerClientTripTime = GetWorld()->GetTimeSeconds() - TimeOfServerRecievedClientRequest;
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
		int32 Minutes = FMath::FloorToInt(Time / 60.f);
		int32 Seconds = Time - Minutes * 60;
		FString TimerText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CHPlayerHUD->PlayerOverlay->MatchTimerText->SetText(FText::FromString(TimerText));
	}
}

void ACHPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
}

void ACHPlayerController::OnRep_MatchState()
{
}
