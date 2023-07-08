// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class CONTACTHOSTILE_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;

	 /**
		 @brief SetDisplayText - Setter for DisplayText member.
		 @param TextToDisplay - The text to set to.
	 **/
	void SetDisplayText(FString TextToDisplay);

	 /**
		 @brief ShowPlayerNetRole gets the player's net role
		 @param InPawn - APawn to get NetRole from.
	 **/
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

protected:

	 /**
		 @brief OnLevelRemovedFromWorld - called when changing levels. Removes widget from viewport.
		 @param InLevel - Pointer to current Level
		 @param InWorld - Pointer to current World
	 **/
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;
};
