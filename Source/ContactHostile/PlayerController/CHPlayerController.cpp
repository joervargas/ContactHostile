// Fill out your copyright notice in the Description page of Project Settings.


#include "CHPlayerController.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "ContactHostile/HUD/CHPlayerHUD.h"
#include "ContactHostile/HUD/PlayerOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void ACHPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CHPlayerHUD = Cast<ACHPlayerHUD>(GetHUD());
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
