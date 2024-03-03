// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/BoxComponent.h"


void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AContactHostileCharacter* OwnerCharacter = Cast<AContactHostileCharacter>(GetOwner());
	//AContactHostileCharacter* CHCharacter = Cast<AContactHostileCharacter>(OtherActor);
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->GetController();
		if (OwnerController)
		{
			//CHCharacter->SetDamageHitResult(Hit);
			//CHCharacter->SetDamageImpulseScaler(Damage * 1000);
			//UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
			
			UGameplayStatics::ApplyPointDamage(OtherActor, Damage, Hit.ImpactNormal, Hit, OwnerController, this, UDamageType::StaticClass());
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
