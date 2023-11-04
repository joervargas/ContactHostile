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

	PlayerHUD = Cast<ACHPlayerHUD>(GetHUD());
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
	PlayerHUD = PlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && 
		PlayerHUD->PlayerOverlay && 
		PlayerHUD->PlayerOverlay->HealthBar && 
		PlayerHUD->PlayerOverlay->HealthText;

	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		PlayerHUD->PlayerOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		PlayerHUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
	}

}

void ACHPlayerController::SetHUDScore(float Score)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->PlayerOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ACHPlayerController::SetHUDKilled(int32 KilledCount)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ACHPlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->KilledAmount;

	if (bHUDValid)
	{
		FString KilledText = FString::Printf(TEXT("%d"), KilledCount);
		PlayerHUD->PlayerOverlay->KilledAmount->SetText(FText::FromString(KilledText));
	}
}
