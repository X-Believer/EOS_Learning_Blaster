// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ShotGun.h"

#include "BlasterComponent/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/BlasterPlayerController.h"
#include "Sound/SoundCue.h"

void AShotGun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	AController* InstigatorController = OwnerPawn->GetController();
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		const FTransform MuzzleFlashSocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = MuzzleFlashSocketTransform.GetLocation();
		
		TMap<ABlasterCharacter*, uint32> HitMap;
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter)
			{
				if (HitMap.Contains(BlasterCharacter))	
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}
			if (ImpactParticle)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticle,
					HitTarget,
					FireHit.ImpactNormal.Rotation());
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-5.f, 5.f));
			}
		}
		TArray<ABlasterCharacter*> HitCharacters;
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						HitPair.Key, 
						Damage * HitPair.Value, 
						InstigatorController, 
						this, 
						UDamageType::StaticClass());
				}
				HitCharacters.AddUnique(HitPair.Key);
			}
		}
		if (!HasAuthority() && bUseServerSideRewind)
		{
			OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : OwnerCharacter.Get();
			OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : OwnerController.Get();
			if (OwnerCharacter && OwnerController && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled())
			{
				OwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters, 
					Start, 
					HitTargets, 
					OwnerController->GetServerTime() - OwnerController->SingleTripTime);
			}
		}
	}
}

void AShotGun::ShotgunTraceWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket == nullptr) return;
	const FTransform MuzzleFlashSocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = MuzzleFlashSocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
		
		HitTargets.Add(ToEndLoc);
	}
}

