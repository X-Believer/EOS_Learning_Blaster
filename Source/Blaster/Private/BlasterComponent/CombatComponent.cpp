// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Sound/SoundCue.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (OwnerCharacter->GetFollowCamera())
		{
			DefaultFOV = OwnerCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		
		if (OwnerCharacter->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	OwnerController = (OwnerController == nullptr) ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController && OwnerCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrossHair(HitResult);
		HitTarget = HitResult.ImpactPoint;
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedCurrentWeaponAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{
	if (OwnerCharacter == nullptr || InWeapon == nullptr) return;
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	
	EquippedWeapon = InWeapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	if (const USkeletalMeshSocket* RightHandSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		RightHandSocket->AttachActor(EquippedWeapon, OwnerCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetHUDAmmo();
	
	// Set weapon ammo
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedCurrentWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
	
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, OwnerCharacter->GetActorLocation());
	}
	
	if (EquippedWeapon->AmmoRunOut())
	{
		Reload();
	}
	
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnerCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && OwnerCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		if (const USkeletalMeshSocket* RightHandSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
		{
			RightHandSocket->AttachActor(EquippedWeapon, OwnerCharacter->GetMesh());
		}
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerCharacter->bUseControllerRotationYaw = true;
		
		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, OwnerCharacter->GetActorLocation());
		}
	}
}

void UCombatComponent::Reload()
{
	if (CarriedCurrentWeaponAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::FinishReloading()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	if (OwnerCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireInputPressed)
	{
		Fire();
	}
}

void UCombatComponent::HandleReload()
{
	OwnerCharacter->PlayReloadMontage();
}

void UCombatComponent::UpdateAmmoValues()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()) && ReloadAmount > 0)
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedCurrentWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		EquippedWeapon->AddAmmo(ReloadAmount);
	}
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireInputPressed)
		{
			Fire();
		}
		break;
	default:
		break;
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);
		CrosshairShootFactor = 0.7f;
		StartFireTimer();
	}
}

void UCombatComponent::FirePressed(bool bFirePressed)
{
	bFireInputPressed = bFirePressed;
	
	if (bFireInputPressed)
	{
		Fire();
	}
}

void UCombatComponent::OnRep_CarriedCurrentWeaponAmmo()
{
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || OwnerCharacter == nullptr) return;
	OwnerCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;
	if (EquippedWeapon == nullptr || OwnerCharacter == nullptr) return;
	if (bFireInputPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->AmmoRunOut())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->AmmoRunOut() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	MulticastFire(TraceTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (OwnerCharacter && CombatState == ECombatState::ECS_Unoccupied)
	{
		OwnerCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceTarget);
	}
}

void UCombatComponent::SetAiming(bool InbAiming)
{
	bAiming = InbAiming;
	ServerSetAiming(InbAiming);
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool InbAiming)
{
	bAiming = InbAiming;
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TraceUnderCrossHair(FHitResult& HitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	
	if (UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), 
		CrosshairLocation, 
		CrosshairWorldPosition, 
		CrosshairWorldDirection))
	{
		FVector Start = CrosshairWorldPosition;
		
		if (OwnerCharacter)
		{
			float DistanceToCharacter = (OwnerCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 50.f);
		}
		
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		
		GetWorld()->LineTraceSingleByChannel(
			HitResult, 
			Start, 
			End, 
			ECollisionChannel::ECC_Visibility);
		if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (OwnerCharacter == nullptr || OwnerCharacter->Controller == nullptr) return;
	
	if (OwnerController)
	{
		OwnerHUD = OwnerHUD == nullptr ? Cast<ABlasterHUD>(OwnerController->GetHUD()) : OwnerHUD.Get();
		if (OwnerHUD)
		{
			HUDPackage.CrosshairsCenter = EquippedWeapon ? EquippedWeapon->CrosshairCenter : nullptr;
			HUDPackage.CrosshairsLeft = EquippedWeapon ? EquippedWeapon->CrosshairLeft : nullptr;
			HUDPackage.CrosshairsRight = EquippedWeapon ? EquippedWeapon->CrosshairRight : nullptr;
			HUDPackage.CrosshairsTop = EquippedWeapon ? EquippedWeapon->CrosshairTop : nullptr;
			HUDPackage.CrosshairsBottom = EquippedWeapon ? EquippedWeapon->CrosshairBottom : nullptr;
			
			// Calculate crosshair spread
			FVector2D WalkSpeedRange(0.f, BaseWalkSpeed);
			FVector2D SpreadRange(0.f, 1.f);
			FVector Velocity = OwnerCharacter->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, SpreadRange, Velocity.Size());
			if (OwnerCharacter->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, 0.f, DeltaTime, 40.f);
			
			HUDPackage.CrosshairsSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootFactor;
			
			OwnerHUD->SetHUDPackage(HUDPackage);
		}
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, CarriedAmmo);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (OwnerCharacter && OwnerCharacter->GetFollowCamera())
	{
		OwnerCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}
