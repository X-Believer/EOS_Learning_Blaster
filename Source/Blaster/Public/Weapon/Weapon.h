// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class USoundCue;
class ABlasterPlayerController;
class ABlasterCharacter;
class ACasing;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	
	EFT_MAX UMETA(DisplayName = "DefaultMAX")
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
	void SetHUDAmmo();
	virtual void Fire(const FVector& HitTarget);
	void Dropped();
	void AddAmmo(int32 AmmoAmount);
	FVector TraceEndWithScatter(const FVector& HitTarget) const;
	
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
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<USoundCue> EquipSound;
	
	/*
	 * Custom depth
	 */
	UFUNCTION()
	void EnableCustomDepth(bool bEnable);
	
	bool bDestroyWeapon = false;
	
	UPROPERTY(EditAnywhere)
	EFireType FireType;
		
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	bool bUseScatter = false;
	
	
protected:
	virtual void BeginPlay() override;
	virtual void OnRep_Owner() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnEquippedSecondary();
	virtual void OnDropped();
	
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
	
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
		
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> OwnerController;
		
	/*
	 * Trace end with scatter
	 */
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float DistanceToSphere = 800.f;
	
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float SphereRadius = 75.f;
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(Replicated, EditAnywhere, Category="Lag Compensation")
	bool bUseServerSideRewind = false;
	
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
	
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 Ammo;
	
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 InAmmo);
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	int32 MagCapacity;
	
	// The unprocessed server requests for ammo
	int32 Sequence = 0;
	
	UFUNCTION()
	void OnRep_WeaponState();
	
	void SpendRound();

public:	
	void SetWeaponState(const EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; } 
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	bool AmmoRunOut() const;
	bool IsAmmoFull() const;
};
