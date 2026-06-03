// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/CombatState.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "CombatComponent.generated.h"

class AProjectile;
class ABlasterHUD;
class ABlasterPlayerController;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* InWeapon);
	void SwapWeapon();
	void EquipPrimaryWeapon(AWeapon* InWeapon);
	void EquipSecondaryWeapon(AWeapon* InWeapon);
	void Reload();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
	UFUNCTION(BlueprintCallable)
	void FinishSwap();
	
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapon();
	
	void FirePressed(bool bFireInputPressed);
	
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	
	UFUNCTION(BlueprintCallable)
	void JumpToShotgunEnd();
	
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();
	
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
	
	void ShowAttachedGrenade(bool bShow);
	
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	bool bLocallyReload = false;
	
protected:
	void SetAiming(bool InbAiming);
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool InbAiming);
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void ReloadEmptyWeapon();

	/*
	 * Fire
	 */
	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceTarget);
	
	UFUNCTION(Server, Reliable)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	/*
	 * Reload
	 */
	
	UFUNCTION(Server, Reliable)
	void ServerReload();
	
	void HandleReload();
	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();

	void TraceUnderCrossHair(FHitResult& HitResult);
	
	UFUNCTION()
	void SetHUDCrosshairs(float DeltaTime);
	
	int32 AmountToReload();
	
	/*
	 * Grenade
	 */
	void ThrowGrenade();
	
	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> GrenadeClass;
	
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> OwnerController;
	
	UPROPERTY()
	TObjectPtr<ABlasterHUD> OwnerHUD;
	
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;
	
	UPROPERTY(ReplicatedUsing=OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;
	
	UPROPERTY(ReplicatedUsing=OnRep_Aiming)
	bool bAiming;
	
	UPROPERTY()
	bool bAimButtonPressed = false;
	
	UFUNCTION()
	void OnRep_Aiming();
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float BaseWalkSpeed;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float AimWalkSpeed;
	
	bool bFireInputPressed;
	
	FVector HitTarget;
	
	FHUDPackage HUDPackage;
	
	/*
	 * HUD and crosshair
	 */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootFactor;
	
	/*
	 * Aiming and FOV
	 */
	float DefaultFOV;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float ZoomedFOV = 30.f;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float ZoomInterpSpeed = 20.f;
	
	float CurrentFOV;
	
	void InterpFOV(float DeltaTime);
	
	/*
	 * Automatic fire
	 */
	
	FTimerHandle FireTimer;

	bool bCanFire = true;
	
	UPROPERTY(ReplicatedUsing=OnRep_CarriedCurrentWeaponAmmo)
	int32 CarriedCurrentWeaponAmmo;
	
	UFUNCTION()
	void OnRep_CarriedCurrentWeaponAmmo();
	
	UPROPERTY()
	TMap<EWeaponType, int32> CarriedAmmoMap;
	
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;
	
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 45;
	
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 4;
	
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 15;
	
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 20;
	
	UPROPERTY(EditAnywhere)
	int32 StartingShotGunAmmo = 10;
	
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 20;
	
	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 1;
	
	UPROPERTY(ReplicatedUsing=OnRep_Grenades)
	int32 CarriedGrenade = 3;
	
	UPROPERTY(EditAnywhere)
	int32 MaxGrenade = 4;
	
	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	
	UFUNCTION()
	void OnRep_CombatState();
	UFUNCTION()
	void OnRep_Grenades();
	void UpdateHUDGrenades();
	void InitializeCarriedAmmo();
	void StartFireTimer();
	void FireTimerFinished();
	bool CanFire();
	
public:
	FORCEINLINE int32 GetCarriedGrenade() const {return CarriedGrenade;}
	bool ShouldSwapWeapons();
};
