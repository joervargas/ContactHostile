// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "ContactHostile/Character/ContactHostileCharacter.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AContactHostileCharacter* CHCharacter = Cast<AContactHostileCharacter>(GetOwner());
	if (CHCharacter)
	{
		AController* CharacterController = CHCharacter->Controller;
		if (CharacterController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, CharacterController, this, UDamageType::StaticClass());
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
