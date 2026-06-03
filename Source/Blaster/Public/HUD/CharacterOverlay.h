// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;
	
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreNum;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatNum;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoNum;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoNum;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesNum;
	
	UPROPERTY(meta = (BindWidget))
	UImage* HighPingImage;
	
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HighPingAnim;
};
