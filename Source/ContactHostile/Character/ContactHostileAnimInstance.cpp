// Fill out your copyright notice in the Description page of Project Settings.


#include "ContactHostileAnimInstance.h"
#include "ContactHostileCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ContactHostile/ContactHostileComponents/CombatComponent.h"
#include "ContactHostile/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
//#include "Engine/KismetAnimationLibrary.h"


void UContactHostileAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CHCharacter = Cast<AContactHostileCharacter>(TryGetPawnOwner());
}

void UContactHostileAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (CHCharacter == nullptr)
	{
		CHCharacter = Cast<AContactHostileCharacter>(TryGetPawnOwner());
	}
	if (CHCharacter == nullptr) return;

	//FVector Velocity = CHCharacter->GetHor
	//Velocity.Z = 0.f;
	Speed = CHCharacter->GetSpeed();
	
	DirectionAxis = CalculateDirection(CHCharacter->GetHorizontalVelocity(), CHCharacter->GetActorRotation());

	//ForwardMovement = CalcForwardMovement(Velocity);

	bIsInAir = CHCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = CHCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bIsSprinting = CHCharacter->SprintReady();

	bIsCrouched = CHCharacter->CrouchReady();
	
	bIsProne = CHCharacter->ProneReady();

	bWeaponEquipped = CHCharacter->IsWeaponEquipped();

	bAiming = CHCharacter->AimReady();

	AO_Yaw = CHCharacter->GetAO_Yaw();
	AO_Pitch = CHCharacter->GetAO_Pitch();

	TurningInPlace = CHCharacter->GetTurningInPlace();

	//FRotator AimRotation = CHCharacter->GetBaseAimRotation();
	//FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(CHCharacter->GetVelocity());
	//YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;


	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = CHCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float InterpLean = FMath::FInterpTo(Lean, Target, DeltaTime, 3.f);

	Lean = FMath::Clamp(InterpLean, -90.f, 90.f);

	bLocallyControlled = CHCharacter->IsLocallyControlled();

	if (bWeaponEquipped && CHCharacter->GetMesh())
	{
		const AWeapon* EquippedWeapon = CHCharacter->GetCombat()->GetEquippedWeapon();
	
		if (bLocallyControlled)
		{
			LookAtLocation = CHCharacter->GetAimLocation();
		}

		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPostion;
		FRotator OutRotation;
		// Transform LeftHand from World Space to RightHand's bone space.  Populate the OutPosition and OutRotation
		CHCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPostion, OutRotation);
		LeftHandTransform.SetLocation(OutPostion);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		//FVector MuzzleOut(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::Y));
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleOut * 1000.f, FColor::Red);
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), CHCharacter->GetAimLocation(), FColor::Orange);
	}
}