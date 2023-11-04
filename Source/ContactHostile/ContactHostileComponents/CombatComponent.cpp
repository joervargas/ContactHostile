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
#include "TimerManager.h"



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
	if (CHCharacter)
	{
		SpawnDefaultWeapon();
	}

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
		if (HUD)
		{
			SetHUDCrosshairSpread(DeltaTime);
		}
		InterpZoomFOV(DeltaTime);
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

		if (CHCharacter)
		{
			float DistanceToCharacter = (CHCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
			//DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}

		FVector End = Start + (CrosshairWorldDirection * TRACE_LENGTH);

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start, End,
			ECollisionChannel::ECC_Visibility
		);
		if (HUD)
		{
			if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
			{
				HUD->SetHUDPackageCrosshairColor(FLinearColor::Red);
			} else
			{
				HUD->SetHUDPackageCrosshairColor(FLinearColor::White);
			}
		}
		//if (TraceHitResult.bBlockingHit)
		//{
		//	//DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12.f, 12, FColor::Red);
		//}
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

void UCombatComponent::AttachToRightHand(AActor* Item)
{
	if (CHCharacter == nullptr || CHCharacter->GetMesh() == nullptr || Item == nullptr) { return; }

	//EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = CHCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(Item, CHCharacter->GetMesh());
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

void UCombatComponent::HideWeaponForOwner(bool bHide)
{
	if (EquippedWeapon && EquippedWeapon->GetWeaponMesh())
	{
		EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(bHide);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{

	if (CHCharacter == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = CHCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));

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
	} else
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

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && CHCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = CHCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, CHCharacter->GetMesh());
		}
	}
}

void UCombatComponent::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass && CHCharacter)
	{
		//if (CHCharacter->HasAuthority())
		//{
			//if (EquippedWeapon == nullptr)
			//{
				AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
				EquipWeapon( SpawnedWeapon );
				//if (EquippedWeapon != SpawnedWeapon)
				//{
				//	SpawnedWeapon->Destroy();
				//}
			//}
			//MulticastSpawnDefaultWeapon();
			//ServerSpawnDefaultWeapon();
			//ClientSpawnDefaultWeapon();
			//EquipWeapon(DefaultWeaponClass.GetDefaultObject());
		//}
		//else 
		//{
			//ServerSpawnDefaultWeapon();
			//ClientSpawnDefaultWeapon();
		//}
		//if (EquippedWeapon == nullptr && CHCharacter->HasAuthority())
		//{
		//	ServerSpawnDefaultWeapon();
		//}
	}
}

void UCombatComponent::ServerSpawnDefaultWeapon_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("ServerSpawnDefaultWeapon() Called)"));
	//if (DefaultWeaponClass && CHCharacter)
	//{
	//	if (EquippedWeapon == nullptr)
	//	{
			EquipWeapon( GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass) );
	//	}
	//}
	//MulticastSpawnDefaultWeapon();
}

void UCombatComponent::MulticastSpawnDefaultWeapon_Implementation()
{
//	if (DefaultWeaponClass && CHCharacter)
//	{
//		FActorSpawnParameters SpawnParameters;
//		SpawnParameters.Owner = CHCharacter;
//		//FTransform SpawnTransform = CHCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"))->GetSocketTransform();
		AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		EquipWeapon( SpawnedWeapon );
//	}
}

void UCombatComponent::ClientSpawnDefaultWeapon_Implementation()
{
	if (DefaultWeaponClass && CHCharacter)
	{
		//FActorSpawnParameters SpawnParameters;
		//SpawnParameters.Owner = CHCharacter;

		AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		EquipWeapon(SpawnedWeapon);
		//if (EquippedWeapon != nullptr)
		//{
		//	EquipWeapon(EquippedWeapon);
		//}
	}
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
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (bCanFire)
	{
		bCanFire = false;
		ServerFire(HitResult.ImpactPoint);
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}

		FireTimerStart();
	}
}

void UCombatComponent::FireTimerStart()
{
	if (EquippedWeapon == nullptr || CHCharacter == nullptr) { return; }
	CHCharacter->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) { return; }
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bFullAutomatic)
	{
		Fire();
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

//void UCombatComponent::OnRep_EquippedWeapon()
//{
//	if (!CHCharacter || !EquippedWeapon) return;
//	AttachToRightHand(EquippedWeapon);
//	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
//	EquippedWeapon->SetOwner(CHCharacter);
//}
