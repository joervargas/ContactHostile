// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CHPlayerHUD.generated.h"


USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:

	class UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairSpread;
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API ACHPlayerHUD : public AHUD
{
	GENERATED_BODY()

public:

	// Called every frame
	virtual void DrawHUD() override;

private:

	UPROPERTY(EditAnywhere)
		float CrosshairSpreadMax = 16.f;

	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, FLinearColor CrosshairColor);

public:

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE void SetHUDPackageCrossHairSpread(const float Spread) { HUDPackage.CrosshairSpread = Spread; }
	FORCEINLINE void SetHUDPackageCrosshairColor(const FLinearColor Color) { HUDPackage.CrosshairsColor = Color; }

};
