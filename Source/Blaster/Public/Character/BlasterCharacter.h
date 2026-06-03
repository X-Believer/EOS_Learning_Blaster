// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterTypes/CombatState.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

class ULagCompensationComponent;
class UBoxComponent;
class UBuffComponent;
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
	void RotateInPlace(float DeltaTime);
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	
	void SpawnDefaultWeapon();
	
	void EquipWeapon();
	void Reload();
	virtual void Jump() override;
	void AimBegin();
	void AimEnd();
	void FireBegin();
	void FireEnd();
	void CalculateAO_Pitch();
	void ThrowGrenade();
	
	virtual void OnRep_ReplicatedMovement() override;
	float CalculateSpeed();
	
	UFUNCTION()
	void Elim();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	
	virtual void Destroyed() override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShow);
		
	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;
	
	bool bFinishSwaping = false;

protected:
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);
	
	void DropOrDestroyWeapon(AWeapon* InWeapon);
	void DropOrDestroyWeapons();

	// Poll for HUD element
	UFUNCTION()
	void PollInit();
	
	/*
	 * Hit boxes used for server rewind
	 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Head;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Pelvis;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Spine_02;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Spine_03;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> UpperArm_L;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> UpperArm_R;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> LowerArm_L;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> LowerArm_R;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Hand_L;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Hand_R;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Backpack;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> Blanket;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Thigh_L;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Thigh_R;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Calf_L;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Calf_R;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Foot_L;
	
	UPROPERTY(EditAnywhere)	
	TObjectPtr<UBoxComponent> Foot_R;
	
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidgetComponent;
	
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBuffComponent> BuffComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<ULagCompensationComponent> LagCompensation;
	
	/*
	 * Anim montages
	 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> FireWeaponMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> SwapMontage;
	
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
	void OnRep_Health(float LastHealth);
	
	/*
	 * Player shield
	 */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 100.f;
	
	UFUNCTION()
	void OnRep_Shield(float LastShield);

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
	
	/*
	 * Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;
	
	/*
	 * Default weapon
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float NewHealth) { Health = NewHealth; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float NewShield) { Shield = NewShield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }
	FORCEINLINE UBuffComponent* GetBuffComponent() const { return BuffComponent; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FVector GetHitTargetLocation();
	AWeapon* GetEquippedWeapon() const;
	ECombatState GetCombatState() const;
	bool IsLocallyReloading();
};
