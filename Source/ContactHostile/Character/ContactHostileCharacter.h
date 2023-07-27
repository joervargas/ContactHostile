// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ContactHostile/CHTypes/TurningInPlace.h"
#include "ContactHostile/CHTypes/InteractWithCrosshairsInterface.h"
#include "ContactHostileCharacter.generated.h"


UENUM(BlueprintType)
enum class ESpeedModes : uint8
{
	ESM_Walk UMETA(DisplayName = "Walk"),
	ESM_Crouch UMETA(DisplayName = "Crouch"),
	ESM_Sprint UMETA(DisplayName = "Sprint"),
	ESM_Prone UMETA(DisplayName = "Prone"),
	ESM_StandAim UMETA(DisplayName = "StandAim"),
	ESM_CrouchAim UMETA(DisplayName = "CrouchAim"),

	ESM_MAX UMETA(DisplayName = "DefaultMAX")
};

class UAnimMontage;

UCLASS()
class CONTACTHOSTILE_API AContactHostileCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	AContactHostileCharacter();

	// Called to replicate members
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	 /**
		 @brief MoveForward Moves character forwards or backwards depending on Value (+ or -).
		 @param Value - Value to move the character by.
	 **/
	void MoveForward(float Value);

	 /**
		 @brief MoveRight Moves the character right or left depending on Value (+ or -).
		 @param Value - Value to move the character by.
	 **/
	void MoveRight(float Value);

	virtual void AddControllerPitchInput(float Value) override;

	virtual void Jump() override;

	void SprintButtonPressed();
	void SprintButtonReleased();

	void EquipButtonPressed();

	void CrouchProneButtonPressed();
	void CrouchProneButtonRepeat();
	void CrouchProneButtonReleased();

	void AimButtonPressed();
	void AimButtonReleased();

	void FireButtonPressed();
	void FireButtonReleased();

	void CalcAimOffset(float DeltaTime);

	void CalcTurnInPlace(float DeltaTime);

private:

	FVector HorizontalVelocity;

	float Speed;

	float ForwardMovement;

	float AO_Yaw; // Aim Offset Yaw
	float AO_Yaw_LastFrame;
	float Interp_AO_Yaw; // for Interping Turning
	float AO_Pitch; // Aim Offset Pitch

	FRotator StartingAimRotation;

	ESpeedModes SpeedMode;

	ETurningInPlace TurningInPlace;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	int64 CrouchProneBtnThreshold = 25;

	bool bCrouchProneBtnRepeatFlag = false;
	int64 CrouchProneBtnHoldTime;

	UPROPERTY(ReplicatedUsing = OnRep_bIsProne)
	bool bIsProne;

	UFUNCTION()
	void OnRep_bIsProne();

	bool bIsGettingToProne;

	bool bIsStandingFromProne;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float PronePitchMax;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float PronePitchMin;

	UPROPERTY(Replicated)
	FVector MeshStartingRelativeLocation;

	UPROPERTY(Replicated)
	FVector MeshProneRelativeLocation;

	FVector MeshCurrentRelativeLocation;

	UPROPERTY(EditAnywhere, Category = CapsuleCollisions, meta = (AllowPrivateAccess = "true"))
	float CapsuleRadius = 34.0f;

	UPROPERTY(EditAnywhere, Category = CapsuleCollisions, meta = (AllowPrivateAccess = "true"))
	float CapsuleStandHalfHeight = 88.0f;

	UPROPERTY(EditAnywhere, Category = CapsuleCollisions, meta = (AllowPrivateAccess = "true"))
	float CapsuleCrouchHalfHeight = 55.0f;

	UPROPERTY(EditAnywhere, Category = CapsuleCollisions, meta = (AllowPrivateAccess = "true"))
	float CapsuleProneHalfHeight = 36.0f;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float AimSpeed;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float ProneSpeed;

	UPROPERTY(EditAnywhere, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float SprintSpeed;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bSprintButtonPressed;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float CameraDistanceThreshold = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage; // Set in Blueprints

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage; // Set in Blueprints

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerSprintButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerSprintButtonReleased();

	void SetProne(bool bProne);

	UFUNCTION(Server, Reliable)
	void ServerSetProne(bool bProne);

	float CalcProneCollisions(float MoveValue);

	bool IsInPitchConstraints(float Value);

	void CalcRelativeLocations();

	UFUNCTION(Client, Reliable)
	void ClientCalcRelativeLocations();

	void CanProne();

	void InterpProneRelativeLocations();

public:	

	
	FORCEINLINE FVector GetHorizontalVelocity() const { return HorizontalVelocity; }
	void SetHorizontalVelocity();

	FORCEINLINE float GetSpeed() const { return Speed; }
	void SetSpeed();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	//FORCEINLINE float GetAimSpeed() const { return AimSpeed; }
	//FORCEINLINE float GetWalkSpeed() const { return WalkSpeed; }
	//FORCEINLINE float GetCrouchSpeed() const { return CrouchSpeed; }
	//FORCEINLINE float GetSprintSpeed() const{ return SprintSpeed; }
	FORCEINLINE bool GetIsSprintButtonPressed() const { return bSprintButtonPressed; }

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	bool AimReady();
	bool CrouchReady();
	bool SprintReady();
	bool ProneReady();

	void SetSpeedMode(ESpeedModes Mode);
	void AssignSpeeds();

	void PlayFireMontage(bool bAiming);

	//FVector GetHitTarget() const;
	FHitResult GetHitResult() const;
	FVector GetAimLocation() const;

	/*
	* Hides the character and weapon mesh when the camera is too close
	*/
	UFUNCTION(BlueprintCallable)
	void HideCharacterIfCameraClose();
};
