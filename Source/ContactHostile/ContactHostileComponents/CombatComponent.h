// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AWeapon;
class AContactHostileCharacter;
class ACHPlayerController;
class ACHPlayerHUD;

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CONTACTHOSTILE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	friend class AContactHostileCharacter;
	UCombatComponent();

	// Called to replicate members
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(AWeapon* WeaponToEquip);

protected:

	virtual void BeginPlay() override;

	void SpawnDefaultWeapon();

	UFUNCTION(Server, Reliable)
	void ServerSpawnDefaultWeapon();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnDefaultWeapon();

	UFUNCTION(Client, Reliable)
	void ClientSpawnDefaultWeapon();

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, unreliable)
	void ServerSetAiming(bool bIsAiming);

	void FireButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/*
	* Line trace from camera to crosshairs
	*/
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairSpread(float DeltaTime);

	void AttachToRightHand(AActor* Item);

private:

	AContactHostileCharacter* CHCharacter;
	ACHPlayerController* PlayerController;
	ACHPlayerHUD* HUD;

	//UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	UPROPERTY(Replicated)
	AWeapon* EquippedWeapon;

	//UFUNCTION()
	//void OnRep_EquippedWeapon();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	// Might not want to replicate this for performance reasons; Animation change barely noticeable.
	UPROPERTY(Replicated)
	bool bAiming;

	bool bFireButtonPressed;

	/**
	*	HUD and crosshairs
	*/
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	//FVector HitTarget;
	FHitResult HitResult;

	/**
	*	Aiming and FOV
	*/

	// FOV when not aiming; Set to camera's FOV in BeginPlay;
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AimZoomFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AimZoomInterpSpeed = 20.f;

	void InterpZoomFOV(float DeltaTime);

public:	


	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	
	UFUNCTION(BlueprintCallable)
	void HideWeaponForOwner(bool bHide);
};
