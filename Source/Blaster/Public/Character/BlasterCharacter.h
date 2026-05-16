// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

class ABlasterPlayerState;
class USoundCue;
class FOnTimelineFloat;
class ABlasterPlayerController;
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
	void PlayElimMontage();
	void UpdateHUDHealth();
	virtual void PossessedBy(AController* NewController) override;
	
	void EquipWeapon();
	virtual void Jump() override;
	void AimBegin();
	void AimEnd();
	void FireBegin();
	void FireEnd();
	void CalculateAO_Pitch();
	
	virtual void OnRep_ReplicatedMovement() override;
	float CalculateSpeed();
	
	UFUNCTION()
	void Elim();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	
	virtual void Destroyed() override;

protected:
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	// Poll for HUD element
	UFUNCTION()
	void PollInit();
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
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;
	
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
	
	/*
	 * Player health
	 */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	
	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;
	
	bool bEliminated = false;
	
	FTimerHandle ElimTimer;
	
	void ElimTimerFinished();
	
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	
	/*
	 * Dissolve Effect
	 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTimelineComponent> DissolveTimeline;;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> DissolveCurve;
	FOnTimelineFloat DissolveTrack;
	
	UPROPERTY(VisibleAnywhere, Category="Eliminated")
	TObjectPtr<UMaterialInstanceDynamic> DynamicDissolveMaterialInstance;;
	
	UPROPERTY(EditAnywhere, Category="Eliminated")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	
	UFUNCTION()
	void StartDissolve();
	
	/*
	 * Elim bot
	 */
	UPROPERTY(EditAnywhere, Category="Eliminated")
	TObjectPtr<UParticleSystem> ElimBotEffect;
	
	UPROPERTY(VisibleAnywhere, Category="Eliminated")
	TObjectPtr<UParticleSystemComponent> ElimBotComponent;
	
	UPROPERTY(EditAnywhere, Category="Eliminated")
	TObjectPtr<USoundCue> ElimBotSound;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FVector GetHitTargetLocation();
	AWeapon* GetEquippedWeapon() const;
};
