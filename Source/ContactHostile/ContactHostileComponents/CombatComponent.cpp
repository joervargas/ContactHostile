// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "ContactHostile/Weapon/Weapon.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "ContactHostile/PlayerController/CHPlayerController.h"
#include "ContactHostile/HUD/CHPlayerHUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	//bReplicates = true; //Not needed in UActorComponent


}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = PlayerController == nullptr ? Cast<ACHPlayerController>(CHCharacter->Controller) : PlayerController;
	
	EquipWeapon(SpawnDefaultWeapon());

	if (CHCharacter->GetFollowCamera())
	{
		DefaultFOV = CHCharacter->GetFollowCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (CHCharacter && CHCharacter->IsLocallyControlled())
	{
		//FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		//HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairSpread(DeltaTime);
		InterpZoomFOV(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairSpread(float DeltaTime)
{
	if (CHCharacter == nullptr || CHCharacter->Controller == nullptr) { return; }

	// // Calculate crosshair spread
	FVector2D MovementSpeedRange(0.f, CHCharacter->GetCharacterMovement()->MaxWalkSpeed);
	FVector2D SpreadRange(0.f, 1.f);
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(MovementSpeedRange, SpreadRange, CHCharacter->GetHorizontalVelocity().Size());

	if (CHCharacter->GetCharacterMovement()->IsFalling()) // In Air
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else // Not in air
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if (bAiming) // Is Aiming
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
	}
	else // Not aiming
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.f);
	}
	CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 20.f);

	if (HUD)
	{
		HUD->SetHUDPackageCrossHairSpread(
			0.5f +
			CrosshairVelocityFactor +
			CrosshairInAirFactor -
			CrosshairAimFactor +
			CrosshairShootingFactor
		);
	}
}

void UCombatComponent::InterpZoomFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetAimFOV(), DeltaTime, EquippedWeapon->GetAimInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, AimZoomInterpSpeed);
	}
	if (CHCharacter->GetFollowCamera())
	{
		CHCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

AWeapon* UCombatComponent::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		//FHitResult HitResult;
		//TraceUnderCrosshairs(HitResult);
		ServerFire(HitResult.ImpactPoint);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition, 
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + (CrosshairWorldDirection * TRACE_LENGTH);
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start, End,
			ECollisionChannel::ECC_Visibility
		);
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUD->SetHUDPackageCrosshairColor(FLinearColor::Red);
		}
		else
		{
			HUD->SetHUDPackageCrosshairColor(FLinearColor::White);
		}
		if (TraceHitResult.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12.f, 12, FColor::Red);
		}
	}
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (CHCharacter)
	{
		CHCharacter->PlayFireMontage(bAiming); // Player fire animation
		EquippedWeapon->Fire(TraceHitTarget); // Weapon fire animation
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{

	if (CHCharacter == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = CHCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (EquippedWeapon->GetWeaponState() == EWeaponState::EWS_Equipped)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponState: Equipped"))
	}
	if (EquippedWeapon->GetWeaponState() == EWeaponState::EWS_Dropped)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponState: Dropped"))
	}

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, CHCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(CHCharacter);
	//EquippedWeapon->ShowPickupWidget(false);

	if (PlayerController == nullptr) { return; }
	HUD = HUD == nullptr ? Cast<ACHPlayerHUD>(PlayerController->GetHUD()) : HUD;
	FHUDPackage HUDPackage;
	if (EquippedWeapon)
	{
		HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
		HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
		HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
		HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
		HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
		HUDPackage.CrosshairSpread = 0.f;
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
	else
	{
		HUDPackage.CrosshairsCenter = nullptr;
		HUDPackage.CrosshairsLeft = nullptr;
		HUDPackage.CrosshairsRight = nullptr;
		HUDPackage.CrosshairsTop = nullptr;
		HUDPackage.CrosshairsBottom = nullptr;
		HUDPackage.CrosshairSpread = 0.f;
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
	HUD->SetHUDPackage(HUDPackage);
}

