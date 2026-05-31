// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterGameMode;
class UCharacterOverlay;
class ABlasterHUD;
class ABlasterCharacter;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABlasterPlayerController();
	void CheckTimeSync(float DeltaTime);
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Pawn() override;
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeatNum(int32 DefeatNum);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	
	// Synced with server world clock
	virtual float GetServerTime();
	// Sync with server clock as soon as possible
	virtual void ReceivedPlayer() override;
	
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleWaitingToStart();
	void HandleCooldown();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetHUDTime();
	void PollInit();
	float ServerClientDelta = 0.f;
	
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;
	
	float TimeSyncRunningTime = 0.f;
	
	/*
	 * Sync time between server and clients
	 */
	
	// Request the current server time, passing in the client's time when request sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	
	// Report the current server time to the client, along with the client's time when request was sent and the server's time when request was received
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedRequest);
	
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);
	
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;
	
	UPROPERTY()
	TObjectPtr<ABlasterGameMode> BlasterGameMode;
	
	UPROPERTY()
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	float LevelStartingTime = 0.f;
	uint32 CountdownInt = 0;
	
	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;
	
	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDScore;
	bool bInitializeScore = false;
	float HUDDefeatNum;
	bool bInitializeDefeatNum = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;
	
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TObjectPtr<ABlasterHUD> BlasterHUD;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> BlasterMappingContext;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ThrowGrenadeAction;
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);
	void Equip(const FInputActionValue& Value);
	void Crouch(const FInputActionValue& Value);
	void AimBegin(const FInputActionValue& Value);
	void AimEnd(const FInputActionValue& Value);
	void FireBegin(const FInputActionValue& Value);
	void FireEnd(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);
	void ThrowGrenade(const FInputActionValue& Value);
	
	UFUNCTION()
	void OnRep_MatchState();
};
