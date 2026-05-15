// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class ACasing;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

class USphereComponent;

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	void ShowPickupWidget(bool bShowWidget) const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Fire(const FVector& HitTarget);
	
	/*
	 * Textures for crosshairs
	 */
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairCenter;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairLeft;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairRight;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairTop;
	
	UPROPERTY(EditAnywhere, Category = "Crosshairs")
	UTexture2D* CrosshairBottom;
	
	/*
	 * Automatic Fire
	 */
		
	UPROPERTY(EditAnywhere, Category="Combat")
	float FireDelay = 0.15f;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	bool bAutomatic = true;
	
	/*
	 * Zoom fov while aiming
	 */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;
	
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f; 
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
	
private:
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState, VisibleAnywhere, Category= "Weapon Properties")
	EWeaponState WeaponState;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<ACasing> CasingClass;
	
	UFUNCTION()
	void OnRep_WeaponState();

public:	
	void SetWeaponState(const EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; } 
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
};
