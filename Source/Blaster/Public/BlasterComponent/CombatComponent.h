// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

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
	
protected:
	void SetAiming(bool InbAiming);
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool InbAiming);
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	void Fire();

	void FirePressed(bool bFireInputPressed);
	
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceTarget);
	
	void TraceUnderCrossHair(FHitResult& HitResult);
	
	UFUNCTION()
	void SetHUDCrosshairs(float DeltaTime);
	
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> OwnerController;
	
	UPROPERTY()
	TObjectPtr<ABlasterHUD> OwnerHUD;
	
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;
	
	UPROPERTY(Replicated)
	bool bAiming;
	
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
	
	void StartFireTimer();
	void FireTimerFinished();
};
