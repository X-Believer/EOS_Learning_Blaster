// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UAnnouncement;
class UCharacterOverlay;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
	
public:
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairsSpread;
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
	
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	
	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;
	
	UPROPERTY()
	TObjectPtr<UAnnouncement> Announcement;
	
	void AddCharacterOverlay();
	void AddAnnouncement();
	
protected:
	virtual void BeginPlay() override;
	
private:
	FHUDPackage HUDPackage;
	
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewPortCenter, FVector2D Spread, FLinearColor Color);
	
	UPROPERTY(EditAnywhere)
	float CrosshairsSpreadMax = 16.f;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
