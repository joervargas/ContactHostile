// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"


AProjectileRocket::AProjectileRocket()
{
	bReplicates = true;
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (TrailSystem)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(), GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);

	}

	CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);

	if (RocketTrailSoundCueLoop && RocketTrailLoopAtenuation)
	{
		RocketTrailSoundComponent = UGameplayStatics::SpawnSoundAttached(
			RocketTrailSoundCueLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f, // Volume multiplier
			1.f, // Pitch multiplier
			0.f, // Start time
			RocketTrailLoopAtenuation,
			nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage, MinimumDammage,
				Hit.Location, InnerBlastRadius, OuterBlastRadius, LinearDamageFalloff,
				UDamageType::StaticClass(), TArray<AActor*>(), this, FiringController
			);
		}
	}

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (RocketMesh) { RocketMesh->SetVisibility(false); }
	if (CollisionBox) { CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (TrailComponent && TrailComponent->GetSystemInstance()) { TrailComponent->GetSystemInstance()->Deactivate(); }
	if (RocketTrailSoundComponent && RocketTrailSoundComponent->IsPlaying())
	{
		RocketTrailSoundComponent->Stop();
	}

	/*Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);*/
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &AProjectileRocket::DestroyTimerFinished, DestroyTime);
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}


void AProjectileRocket::Destroyed()
{

}