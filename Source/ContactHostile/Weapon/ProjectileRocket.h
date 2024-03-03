// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()


public:

	AProjectileRocket();

	virtual void Destroyed() override;

protected:

	UPROPERTY(EditAnywhere)
	float InnerBlastRadius = 200.f; // Inner radius of the radial blast damage

	UPROPERTY(EditAnywhere)
	float OuterBlastRadius = 500.f; // Outer radius of the radial blast damage

	UPROPERTY(EditAnywhere)
	float LinearDamageFalloff = 1.0f; // Exponential falloff rate of damage outside of innerRadius
	
	UPROPERTY(EditAnywhere)
	float MinimumDammage = 10.f; // Minimum amount of damage applicable to other pawns

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailComponent;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	UPROPERTY(EditAnywhere)
	USoundCue* RocketTrailSoundCueLoop;

	UPROPERTY()
	UAudioComponent* RocketTrailSoundComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* RocketTrailLoopAtenuation;

protected:

	virtual void BeginPlay() override;

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void DestroyTimerFinished();

private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;
};
