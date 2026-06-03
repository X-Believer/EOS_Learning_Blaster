// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	virtual void Fire(const FVector& HitTarget) override;
	
protected:
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& FireHit);
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticle;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> HitSound;
	
private:
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> BeamParticle;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> MuzzleFlash;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> FireSound;
};
