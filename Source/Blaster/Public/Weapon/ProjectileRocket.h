// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
	virtual void Destroyed() override;
	
protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> TrailSound;
	
	UPROPERTY()
	TObjectPtr<UAudioComponent> TrailAudioComponent;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> TrailAttenuationSettings;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;
};
