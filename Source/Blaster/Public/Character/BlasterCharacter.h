// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

enum class ETurningInPlace : uint8;
class UCombatComponent;
class AWeapon;
class UWidgetComponent;
class UCameraComponent;
class USpringArmComponent;
class UAnimMontage;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	virtual void PossessedBy(AController* NewController) override;
	
	void EquipWeapon();
	virtual void Jump() override;
	void AimBegin();
	void AimEnd();
	void FireBegin();
	void FireEnd();
	void CalculateAO_Pitch();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();
	
	virtual void OnRep_ReplicatedMovement() override;
	float CalculateSpeed();

protected:
	void AimOffset(float DeltaTime);
	
	void SimProxiesTurn();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidgetComponent;
	
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;
	
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
	
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200;
		
	float AO_Yaw;
	float Interp_AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurningInPlace TurningInPlace;
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMoveReplicated;
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();
	
	void TurnInPlace(float DeltaTime);
	
	void HideCameraIfCharacterClose();
	
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FVector GetHitTargetLocation();
	AWeapon* GetEquippedWeapon() const;
};
