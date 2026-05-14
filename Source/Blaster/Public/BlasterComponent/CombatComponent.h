// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


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
	
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> OwnerCharacter;
	
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;
	
	UPROPERTY(Replicated)
	bool bAiming;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float BaseWalkSpeed;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	float AimWalkSpeed;
	
};
