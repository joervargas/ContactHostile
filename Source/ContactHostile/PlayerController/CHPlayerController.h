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

	void SetHUDTime();

	void PollInit();

	/**
	* Sync time between client and server
	*/

	// Requests the current server time, takes current client time
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports current server time, takes client's request time and time server recieved request
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeOfServerRecievedClientRequest);

	float ClientServerDeltaTime = 0.f; // Difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncTimer = 0.f; // used to calculate when to call TimeSyncFrequency

	void CheckTimeSync(float DeltaTime);

public:

	virtual void ReceivedPlayer() override; // Overridden to Sync time as soon as possible

	virtual void Tick(float DeltaTime) override;

	// Called to replicate members
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // Synced with server world clock

	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDHealth(float Health, float MaxHealth);

	void SetHUDScore(float Score);
	void SetHUDKilled(int32 KilledCount);

	void SetHUDWeaponAmmo(int32 AmmoCount);
	void SetHUDCarriedAmmo(int32 AmmoCount);

	void SetHUDMatchTimer(float Time);

	void OnMatchStateSet(FName State);

	void HandleMatchHasStarted();

private:

	UPROPERTY()
	class ACHPlayerHUD* CHPlayerHUD;

	UPROPERTY()
	class UPlayerOverlay* CHPlayerOverlay;

	float MatchTime = 120.f;

	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	float HUDMakHealth;
	float HUDScore;
	float HUDKilledCount;

};
