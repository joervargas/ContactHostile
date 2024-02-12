// Fill out your copyright notice in the Description page of Project Settings.


#include "ContactHostileCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "ContactHostile/Weapon/Weapon.h"
#include "ContactHostile/ContactHostileComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
//#include "DrawDebugHelpers.h"
#include "ContactHostileAnimInstance.h"
#include "ContactHostile/ContactHostile.h"
#include "ContactHostile/PlayerController/CHPlayerController.h"
#include "ContactHostile/GameMode/CHGameMode.h"
#include "TimerManager.h"
#include "ContactHostile/PlayerState/CHPlayerState.h"
#include "ContactHostile/Weapon/WeaponTypes.h"


// Sets default values
AContactHostileCharacter::AContactHostileCharacter() :
	AimSpeed(125.f),
	WalkSpeed(350.f),
	CrouchSpeed(275),
	ProneSpeed(75.f),
	SprintSpeed(700.f),
	SpeedMode(ESpeedModes::ESM_Walk),
	TurningInPlace(ETurningInPlace::ETIP_NotTurning),
	bIsProne(false),
	bIsGettingToProne(false),
	bIsStandingFromProne(false),
	PronePitchMax(30.f),
	PronePitchMin(-15.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetMesh()->SetIsReplicated(true);
	GetCapsuleComponent()->SetIsReplicated(true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMsh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->NavAgentProps.bCanCrouch = true;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchSpeed;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void AContactHostileCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AContactHostileCharacter, bSprintButtonPressed);
	DOREPLIFETIME(AContactHostileCharacter, bIsProne);
	DOREPLIFETIME(AContactHostileCharacter, MeshStartingRelativeLocation);
	DOREPLIFETIME(AContactHostileCharacter, MeshProneRelativeLocation);
	DOREPLIFETIME(AContactHostileCharacter, Health);
	DOREPLIFETIME(AContactHostileCharacter, bDisableGameplay);
	DOREPLIFETIME_CONDITION(AContactHostileCharacter, OverlappingWeapon, COND_OwnerOnly);

}

void AContactHostileCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->CHCharacter = this;
	}
}

// Called when the game starts or when spawned
void AContactHostileCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	CalcRelativeLocations();
	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AContactHostileCharacter::ReceiveDamage);
	}
}

// Called every frame
void AContactHostileCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHorizontalVelocity();
	SetSpeed();
	AssignSpeeds();
	CalcAimOffset(DeltaTime);
	CanProne();
	InterpProneRelativeLocations();
	HideCharacterIfCameraClose();
	PollInit();
}

// Called to bind functionality to input
void AContactHostileCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AContactHostileCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AContactHostileCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AContactHostileCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("Lookup"), this, &AContactHostileCharacter::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AContactHostileCharacter::Jump);

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AContactHostileCharacter::SprintButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AContactHostileCharacter::SprintButtonReleased);

	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &AContactHostileCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AContactHostileCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction(TEXT("Crouch/Prone"), IE_Pressed, this, &AContactHostileCharacter::CrouchProneButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Crouch/Prone"), IE_Repeat, this, &AContactHostileCharacter::CrouchProneButtonRepeat);
	PlayerInputComponent->BindAction(TEXT("Crouch/Prone"), IE_Released, this, &AContactHostileCharacter::CrouchProneButtonReleased);

	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AContactHostileCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AContactHostileCharacter::AimButtonReleased);

	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AContactHostileCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AContactHostileCharacter::FireButtonReleased);
}

void AContactHostileCharacter::Destroyed()
{
	ACHGameMode* CHGameMode = Cast<ACHGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = CHGameMode && CHGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

//void AContactHostileCharacter::OnRep_ReplicatedMovement()
//{
//	Super::OnRep_ReplicatedMovement();
//	SimProxiesTurn();
//	ProxyTimeSinceLastMovementReplication = 0.f;
//}


void AContactHostileCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) { return; }
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		if (bIsProne)
		{
			Value = CalcProneCollisions(Value);
		}
		ForwardMovement = Value;
		AddMovementInput(GetActorForwardVector() * Value);
	}
}

float AContactHostileCharacter::CalcProneCollisions(float MoveValue)
{
	float Distance = 125.0f * MoveValue;
	FVector LineDistance = GetActorLocation() + (GetActorForwardVector() * Distance);
	FVector StartLocation = GetActorLocation() + (GetActorForwardVector() * (CapsuleStandHalfHeight * MoveValue));
	//DrawDebugLine(GetWorld(), StartLocation, LineDistance, FColor::Purple);
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, LineDistance, ECollisionChannel::ECC_Visibility);
	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("**Line Hit**"));
		return 0.0f;
	}
	return MoveValue;
}

bool AContactHostileCharacter::IsInPitchConstraints(float Value)
{
	// Negative controller input values go up positve values go down.
	if (bIsProne) // In prone position
	{
		const float Pitch = GetBaseAimRotation().Pitch;
		if (Pitch >= PronePitchMax) // Over prone pitch constraints
		{
			if (PronePitchMax - Value < PronePitchMax) // Subtracting controller input value is within constraints
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (Pitch <= PronePitchMin) // Under prone pitch constraints
		{
			if (PronePitchMin - Value > PronePitchMin) // Subtracting controller input value is within constraints
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else // value is within PronePitchMax and PronePitchMin constraints
		{
			return true;
		}
	}
	return true;
}

void AContactHostileCharacter::CalcRelativeLocations()
{

	MeshStartingRelativeLocation = GetMesh()->GetRelativeLocation();
	MeshCurrentRelativeLocation = MeshStartingRelativeLocation;

	const float MeshProneHalfHeightAdjust = GetMesh()->GetRelativeLocation().Z + (CapsuleProneHalfHeight + 15);
	MeshProneRelativeLocation = FVector(MeshStartingRelativeLocation.X, MeshStartingRelativeLocation.Y, MeshProneHalfHeightAdjust);;

	ClientCalcRelativeLocations();

}

void AContactHostileCharacter::ClientCalcRelativeLocations_Implementation()
{
	MeshStartingRelativeLocation = GetMesh()->GetRelativeLocation();
	MeshCurrentRelativeLocation = MeshStartingRelativeLocation;

	const float MeshProneHalfHeightAdjust = GetMesh()->GetRelativeLocation().Z + (CapsuleProneHalfHeight + 15);
	MeshProneRelativeLocation = FVector(MeshStartingRelativeLocation.X, MeshStartingRelativeLocation.Y, MeshProneHalfHeightAdjust);
}

void AContactHostileCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) { return; }
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		AddMovementInput(GetActorRightVector() * Value);
	}
}

void AContactHostileCharacter::AddControllerPitchInput(float Value)
{
	if (IsInPitchConstraints(Value))
	{
		Super::AddControllerPitchInput(Value);
	}
	else
	{
		Super::AddControllerPitchInput(0.f);
	}
}

void AContactHostileCharacter::Jump()
{
	if (bDisableGameplay) { return; }
	if (!bIsProne || bIsCrouched)
	{
		Super::Jump();
	}
}

void AContactHostileCharacter::SprintButtonPressed()
{
	if (bDisableGameplay) { return; }
	if (HasAuthority())
	{
		bSprintButtonPressed = true;
	}
	else
	{
		ServerSprintButtonPressed();
	}
}


void AContactHostileCharacter::ServerSprintButtonPressed_Implementation()
{
	bSprintButtonPressed = true;
}


void AContactHostileCharacter::SprintButtonReleased()
{
	if (bDisableGameplay) { return; }
	if (HasAuthority())
	{
		bSprintButtonPressed = false;
	}
	else
	{
		ServerSprintButtonReleased();
	}
}

void AContactHostileCharacter::ServerSprintButtonReleased_Implementation()
{
	bSprintButtonPressed = false;
}

bool AContactHostileCharacter::SprintReady()
{
	if (bSprintButtonPressed)
	{
		if (bIsCrouched || IsAiming() || Speed <= 0.0f || ForwardMovement < 0.0f)
		{
			return false;
		}
		SetSpeedMode(ESpeedModes::ESM_Sprint);
		return true;
	}
	else
	{
		SetSpeedMode(ESpeedModes::ESM_Walk);
		return false;
	}
}

void AContactHostileCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) { return; }
	if (HasAuthority())
	{
		if (Combat && OverlappingWeapon && OverlappingWeapon->GetWeaponState() != EWeaponState::EWS_Equipped)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
	}
	else
	{
		ServerEquipButtonPressed();
	}
}

void AContactHostileCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) { return; }
	if (Combat)
	{
		Combat->Reload();
	}
}

void AContactHostileCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat && OverlappingWeapon && OverlappingWeapon->GetWeaponState() != EWeaponState::EWS_Equipped)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AContactHostileCharacter::CrouchProneButtonPressed()
{
	if (bDisableGameplay) { return; }
	// Start Button Timer
	CrouchProneBtnHoldTime = FDateTime::Now().GetTicks();
}

void AContactHostileCharacter::CrouchProneButtonRepeat()
{
	if (bDisableGameplay) { return; }
	if (bCrouchProneBtnRepeatFlag) return;

	bCrouchProneBtnRepeatFlag = true;

	UnCrouch();
	SetProne(!bIsProne);

	CrouchProneBtnHoldTime = 0;
}

void AContactHostileCharacter::CrouchProneButtonReleased()
{
	if (bDisableGameplay) { return; }
	if (bCrouchProneBtnRepeatFlag) // Already andled in CrouchProneBtnRepeat function
	{
		bCrouchProneBtnRepeatFlag = false;
		return;
	}

	int64 CurrentMilliSeconds = FDateTime::Now().GetTicks();
	int64 ElapsedTime = (CurrentMilliSeconds - CrouchProneBtnHoldTime) / 100000;

	if (ElapsedTime < CrouchProneBtnThreshold) // Btn not held long enough
	{
		if (bIsCrouched) // Crouched but not prone
		{
			UnCrouch();
			SetProne(false);
			SetSpeedMode(ESpeedModes::ESM_Walk);
		}
		else
		{
			Crouch();
			SetProne(false);
			SetSpeedMode(ESpeedModes::ESM_Crouch);
		}
	}

	if (ElapsedTime >= CrouchProneBtnThreshold) // Btn held long enough for prone
	{

		UnCrouch();
		SetProne(!bIsProne);
	}
	CrouchProneBtnHoldTime = 0;
}

void AContactHostileCharacter::SetProne(bool bProne)
{

	if (bProne)
	{
		bIsProne = bProne;
	}
	else
	{

		if (!bIsCrouched)
		{
			MeshCurrentRelativeLocation = MeshStartingRelativeLocation;
			GetMesh()->SetRelativeLocation(MeshCurrentRelativeLocation, false);
			BaseTranslationOffset = MeshCurrentRelativeLocation;
			GetCapsuleComponent()->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CapsuleStandHalfHeight);
		}

		bIsProne = bProne;
	}
	ServerSetProne(bProne);
}


void AContactHostileCharacter::ServerSetProne_Implementation(bool bProne)
{
	if (bProne)
	{
		bIsProne = bProne;
	}
	else
	{
		if (!bIsCrouched)
		{
			MeshCurrentRelativeLocation = MeshStartingRelativeLocation;
			GetMesh()->SetRelativeLocation(MeshCurrentRelativeLocation, false);
			BaseTranslationOffset = MeshCurrentRelativeLocation;
			GetCapsuleComponent()->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CapsuleStandHalfHeight);
		}
		bIsProne = bProne;
	}
}

void AContactHostileCharacter::OnRep_bIsProne()
{
	if (!bIsProne && !bIsCrouched)
	{
		MeshCurrentRelativeLocation = MeshStartingRelativeLocation;
		GetMesh()->SetRelativeLocation(MeshCurrentRelativeLocation, false);
		BaseTranslationOffset = MeshCurrentRelativeLocation;
		GetCapsuleComponent()->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CapsuleStandHalfHeight);
	}
}

void AContactHostileCharacter::CanProne()
{

	if (bIsProne && MeshCurrentRelativeLocation != MeshProneRelativeLocation &&
		GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() != CapsuleProneHalfHeight)
	{
		bIsGettingToProne = true;
	}
	else
	{
		bIsGettingToProne = false;
		if (!bIsProne && MeshCurrentRelativeLocation != MeshStartingRelativeLocation)
		{
			bIsStandingFromProne = true;
		}
	}

}

void AContactHostileCharacter::InterpProneRelativeLocations()
{

	if (bIsGettingToProne)
	{

		MeshCurrentRelativeLocation = FMath::Lerp(MeshCurrentRelativeLocation, MeshProneRelativeLocation, 0.5f);
		GetMesh()->SetRelativeLocation_Direct(MeshCurrentRelativeLocation);
		BaseTranslationOffset = MeshCurrentRelativeLocation;
		GetCapsuleComponent()->SetCapsuleSize(GetCapsuleComponent()->GetUnscaledCapsuleRadius(), FMath::Lerp(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), CapsuleProneHalfHeight, 0.5f));
	}

}

void AContactHostileCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled()) { return; }

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraDistanceThreshold)
	{
		GetMesh()->SetOwnerNoSee(true);
		if ( Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(true);
		}
	} else {
		GetMesh()->SetOwnerNoSee(false);
		if ( Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
		}
	}
}

bool AContactHostileCharacter::CrouchReady()
{
	if (bIsCrouched)
	{
		SetSpeedMode(ESpeedModes::ESM_Crouch);
	}
	return bIsCrouched;
}

bool AContactHostileCharacter::ProneReady()
{
	if (bIsProne)
	{
		SetSpeedMode(ESpeedModes::ESM_Prone);
	}

	return bIsProne;
}

void AContactHostileCharacter::AimButtonPressed()
{
	if (bDisableGameplay) { return; }
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AContactHostileCharacter::AimButtonReleased()
{
	if (bDisableGameplay) { return; }
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

bool AContactHostileCharacter::AimReady()
{
	bool bIsAiming = IsAiming();
	if (bIsAiming)
	{
		if (bIsCrouched)
		{
			SetSpeedMode(ESpeedModes::ESM_CrouchAim);
		}
		else
		{
			SetSpeedMode(ESpeedModes::ESM_StandAim);
		}
	}
	return bIsAiming;
}

bool AContactHostileCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void AContactHostileCharacter::FireButtonPressed()
{
	if (bDisableGameplay) { return; }
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AContactHostileCharacter::FireButtonReleased()
{
	if (bDisableGameplay) { return; }
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void AContactHostileCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// If a Overlapping weapon is valid but current Weapon pointer is null,
	// hide widget before setting nullpointer;
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon && !bEliminated)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}


void AContactHostileCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool AContactHostileCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}


void AContactHostileCharacter::SetSpeedMode(ESpeedModes Mode)
{
	SpeedMode = Mode;
}

void AContactHostileCharacter::AssignSpeeds()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	switch (SpeedMode)
	{
	case ESpeedModes::ESM_Walk:
		Movement->MaxWalkSpeed = WalkSpeed;
		break;
	case ESpeedModes::ESM_Sprint:
		Movement->MaxWalkSpeed = SprintSpeed;
		break;
	case ESpeedModes::ESM_Crouch:
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
		break;
	case ESpeedModes::ESM_Prone:
		Movement->MaxWalkSpeed = ProneSpeed;
		break;
	case ESpeedModes::ESM_StandAim:
		Movement->MaxWalkSpeed = AimSpeed;
		break;
	case ESpeedModes::ESM_CrouchAim:
		Movement->MaxWalkSpeedCrouched = AimSpeed;
		break;
	default:
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

void AContactHostileCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void AContactHostileCharacter::UpdateHUDHealth()
{
	CHPlayerController = CHPlayerController == nullptr ? Cast<ACHPlayerController>(Controller) : CHPlayerController;
	if (CHPlayerController)
	{
		CHPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AContactHostileCharacter::PollInit()
{
	if (CHPlayerState == nullptr)
	{
		CHPlayerState = GetPlayerState<ACHPlayerState>();
		if (CHPlayerState)
		{
			CHPlayerState->AddToScore(0.f);
			CHPlayerState->AddToKilledCount(0);
		}
	}
}

void AContactHostileCharacter::SetHorizontalVelocity()
{
	HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.f;
}

void AContactHostileCharacter::SetSpeed()
{
	Speed = HorizontalVelocity.Length();
}


void AContactHostileCharacter::CalcAimOffset(float DeltaTime)
{
	if (bDisableGameplay) 
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return; 
	}

	bool bIsInAir = GetCharacterMovement()->IsFalling();
	// Standing still and not in air
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw = AO_Yaw;
		}
		//bUseControllerRotationYaw = true;

		CalcTurnInPlace(DeltaTime);

	}
	// Running or in air
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	}

	CalcAO_Pitch();
}

void AContactHostileCharacter::CalcAO_Pitch()
{
	//AO_Pitch = GetBaseAimRotation().Pitch;
	//if (AO_Pitch > 90.f && !IsLocallyControlled())
	//{
	//	// map pitch from [270, 360) to [-90, 0)
	//	FVector2D InRange(270.f, 360.f);
	//	FVector2D OutRange(-90.f, 0.f);
	//	AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	//}

	if (bIsProne && Speed > 0.f) // if moving in prone
	{
		AO_Pitch = 0.f;
		return;
	}
	// UE network compresses Pitch data. Get around this by turning to a vector and back to a rotation
	AO_Pitch = GetBaseAimRotation().Vector().Rotation().Pitch;
}

//void AContactHostileCharacter::SimProxiesTurn()
//{
//	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }
//
//	bRotateRootBone = false;
//
//	ProxyRotationLastFrame = ProxyRotation;
//	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
//
//	if (FMath::Abs(ProxyYaw) > TurnThreshold)
//	{
//		if (ProxyYaw > TurnThreshold)
//		{
//			TurningInPlace = ETurningInPlace::ETIP_Right;
//		} 
//		else if (ProxyYaw < -TurnThreshold)
//		{
//			TurningInPlace = ETurningInPlace::ETIP_Left;
//		} else {
//			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
//		}
//		return;
//	}
//	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
//}

void AContactHostileCharacter::CalcTurnInPlace(float DeltaTime)
{
	AO_Yaw_LastFrame = AO_Yaw;
	if (GetCharacterMovement()->IsFalling()) { return; }
	if (!bIsProne)
	{
		if (AO_Yaw > 45.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (AO_Yaw < -45.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	} else { // Is in prone position
		if (AO_Yaw > 90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (AO_Yaw < -90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0.f, DeltaTime, 5.f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(AO_Yaw) < 10.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			//bUseControllerRotationYaw = true;
		}
	}
}

void AContactHostileCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	FRotator DamagePlayerLookAt = UKismetMathLibrary::FindLookAtRotation(DamagedActor->GetActorLocation(), DamageCauser->GetActorLocation());
	DamageHitRotation = DamagePlayerLookAt - GetControlRotation();
	UE_LOG(LogTemp, Warning, TEXT("HitRotation: %s"), *DamageHitRotation.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("DamageHitResult: %s"), *DamageHitResult.ImpactPoint.Rotation().ToString());
	//HitRotation.Vector();
	DamageImpulseScaler = Damage * 1000;

	if (Health == 0)
	{
		ACHGameMode* CHGameMode = GetWorld()->GetAuthGameMode<ACHGameMode>();
		if (CHGameMode)
		{
			CHPlayerController = CHPlayerController == nullptr ? Cast<ACHPlayerController>(Controller) : CHPlayerController;
			ACHPlayerController* AttackerController = Cast<ACHPlayerController>(InstigatorController);
			CHGameMode->PlayerEliminated(this, CHPlayerController, AttackerController);
		}
	}
}

void AContactHostileCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		if (bAiming) // Aiming
		{
			SectionName = FName("Rifle_Ironsights");
		}
		else // Not Aiming
		{
			SectionName = FName("Rifle_Shoulder");
		}
		//SectionName = bAiming ? FName("Rifle_Ironsights") : FName("Rifle_Shoulder");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AContactHostileCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
		//;
	}
}

void AContactHostileCharacter::StopReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Stop(0.1f, ReloadMontage);
	}
}

void AContactHostileCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName;
		if (DamageHitRotation.Yaw < 20.f && DamageHitRotation.Yaw > -20.f)
		{
			SectionName = FName("HitReact_Front");
		} 
		else if (DamageHitRotation.Yaw < 120.f || DamageHitRotation.Yaw < -120.f)
		{
			SectionName = FName("HitReact_Back");
		}
		else if (DamageHitRotation.Yaw < -20.f && DamageHitRotation.Yaw > -120.f)
		{
			SectionName = FName("HitReact_Left");
		}
		else if (DamageHitRotation.Yaw < 120.f && DamageHitRotation.Yaw > 20.f)
		{
			SectionName = FName("HitReact_Right");
		} else {
			SectionName = FName("HitReact_Front");
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
	//UE_LOG()
	//PlayAnimMontage(HitReactMontage, 1.0f, SectionName);
}

void AContactHostileCharacter::PlayDeathMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) { return; }

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		//FName SectionName = FName("Default");
		//AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AContactHostileCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
		SetOverlappingWeapon(nullptr);
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AContactHostileCharacter::RespawnTimerFinished, RespawnDelay);
}

void AContactHostileCharacter::MulticastElim_Implementation()
{
	if (CHPlayerController) { CHPlayerController->SetHUDWeaponAmmo(0); }
	bEliminated = true;

	GetMesh()->SetSimulatePhysics(true);
	if (GetCharacterMovement()) { GetCharacterMovement()->DisableMovement(); }
	if (CHPlayerController) 
	{ 
		bDisableGameplay = true;
		if (Combat) { Combat->FireButtonPressed(false); }
	}

	FVector DamageVector = DamageHitRotation.Vector();
	GetMesh()->AddImpulse((-DamageVector.GetSafeNormal()) * DamageImpulseScaler);
}


void AContactHostileCharacter::RespawnTimerFinished()
{
	ACHGameMode* CHGameMode = GetWorld()->GetAuthGameMode<ACHGameMode>();
	CHGameMode->RequestRespawn(this, CHPlayerController);
}

FHitResult AContactHostileCharacter::GetFireHitResult() const
{
	if (Combat == nullptr) { return FHitResult(); }

	return Combat->HitResult;
}

FVector AContactHostileCharacter::GetAimLocation() const
{
	if (Combat == nullptr) { return FVector(); }
	
	/*return Combat->HitResult.bBlockingHit ? Combat->HitResult.ImpactPoint : Combat->HitResult.TraceEnd;*/
	return Combat->HitResult.TraceEnd;
}

ECombatState AContactHostileCharacter::GetCombatState() const
{
	if (Combat == nullptr) { return ECombatState::ECS_MAX; }
	return Combat->CombatState;
}
