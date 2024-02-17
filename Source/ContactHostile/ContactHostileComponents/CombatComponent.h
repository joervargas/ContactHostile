// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ContactHostile/Weapon/WeaponTypes.h"
#include "ContactHostile/CHTypes/CombatState.h"
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
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void SetAiming(bool bIsAiming);

	void FireButtonPressed(bool bPressed);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void SpawnDefaultWeapon();

	UFUNCTION(Server, unreliable)
	void ServerSetAiming(bool bIsAiming);

	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/*
	* Line trace from camera to crosshairs
	*/
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairSpread(float DeltaTime);

	void SetHUDCrosshairs();

	void DropEquippedWeapon();

	void AttachToRightHand(AActor* Item);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();

private:

	UPROPERTY()
	AContactHostileCharacter* CHCharacter;
	
	UPROPERTY()
	ACHPlayerController* PlayerController;
	
	UPROPERTY()
	ACHPlayerHUD* HUD;

	//UPROPERTY(Replicated)
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(EditAnywhere, Category = Weapon, meta = (AllowPrivateAccess = "true"))
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

	/*
	*  Automatic Fire
	*/
	FTimerHandle FireTimer;

	bool bCanFire = true;

	void FireTimerStart();
	void FireTimerFinished();

	bool CanFire();

	// Carried Ammo for the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

public:	


	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	
	UFUNCTION(BlueprintCallable)
	void HideWeaponForOwner(bool bHide);
};
