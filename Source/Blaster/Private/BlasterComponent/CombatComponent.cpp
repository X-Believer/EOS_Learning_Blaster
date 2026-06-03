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
#include "Weapon/Projectile.h"
#include "Weapon/ShotGun.h"
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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedCurrentWeaponAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, CarriedGrenade);
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (OwnerCharacter == nullptr || OwnerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	if (const USkeletalMeshSocket* RightHandSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		RightHandSocket->AttachActor(ActorToAttach, OwnerCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (OwnerCharacter == nullptr || OwnerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	if (const USkeletalMeshSocket* LeftHandSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("LeftHandSocket")))
	{
		LeftHandSocket->AttachActor(ActorToAttach, OwnerCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (OwnerCharacter == nullptr || OwnerCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	if (const USkeletalMeshSocket* BackpackSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("BackpackSocket")))
	{
		BackpackSocket->AttachActor(ActorToAttach, OwnerCharacter->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedCurrentWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (OwnerCharacter && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, OwnerCharacter->GetActorLocation());
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->AmmoRunOut())
	{
		Reload();
	}
}

void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{
	if (OwnerCharacter == nullptr || InWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(InWeapon);
	}
	else
	{
		EquipPrimaryWeapon(InWeapon);
	}
	
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnerCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::SwapWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied || OwnerCharacter == nullptr) return;
	
	OwnerCharacter->PlaySwapMontage();
	OwnerCharacter->bFinishSwaping = false;
	CombatState = ECombatState::ECS_SwapingWeapons;
	
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;
	
	if (SecondaryWeapon) SecondaryWeapon->EnableCustomDepth(false);
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* InWeapon)
{
	if (InWeapon == nullptr) return;
	DropEquippedWeapon();
	EquippedWeapon = InWeapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(InWeapon);
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* InWeapon)
{
	if (InWeapon == nullptr) return;
	SecondaryWeapon = InWeapon;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	PlayEquipWeaponSound(InWeapon);
	
	if (SecondaryWeapon == nullptr) return;
	SecondaryWeapon->SetOwner(OwnerCharacter);
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && OwnerCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerCharacter->bUseControllerRotationYaw = true;
		PlayEquipWeaponSound(EquippedWeapon);
		EquippedWeapon->SetHUDAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (EquippedWeapon && OwnerCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

void UCombatComponent::Reload()
{
	if (CarriedCurrentWeaponAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsAmmoFull() && !bLocallyReload)
	{
		ServerReload();
		HandleReload();
		bLocallyReload = true;
	}
}

void UCombatComponent::FinishReloading()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	bLocallyReload = false;
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

void UCombatComponent::FinishSwap()
{
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	OwnerCharacter->bFinishSwaping = true;
	if (SecondaryWeapon) SecondaryWeapon->EnableCustomDepth(true);
}

void UCombatComponent::FinishSwapAttachWeapon()
{
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);
	
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::HandleReload()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->PlayReloadMontage();
	}
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

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedCurrentWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
	EquippedWeapon->AddAmmo(1);
	bCanFire = true;
	if (EquippedWeapon->IsAmmoFull() || CarriedCurrentWeaponAmmo <= 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && OwnerCharacter->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"), OwnerCharacter->GetReloadMontage());
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (OwnerCharacter && GrenadeClass && OwnerCharacter->GetAttachedGrenade())
	{
		const FVector StartLocation = OwnerCharacter->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerCharacter;
		SpawnParams.Instigator = OwnerCharacter;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass, 
				StartLocation, 
				ToTarget.Rotation(), 
				SpawnParams);
		}
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShow)
{
	if (OwnerCharacter && OwnerCharacter->GetAttachedGrenade())
	{
		OwnerCharacter->GetAttachedGrenade()->SetVisibility(bShow);
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->AmmoRunOut() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_Reloading;
	if (!OwnerCharacter->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireInputPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_ThrowGrenade:
		if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
		{
			OwnerCharacter->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwapingWeapons:
		if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
		{
			OwnerCharacter->PlaySwapMontage();
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
		if (EquippedWeapon)
		{
			CrosshairShootFactor = 0.7f;

			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			default:
				break;
			}
		}
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && OwnerCharacter)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!OwnerCharacter->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && OwnerCharacter)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!OwnerCharacter->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotGun* ShotGun = Cast<AShotGun>(EquippedWeapon);
	if (ShotGun && OwnerCharacter)
	{
		TArray<FVector_NetQuantize> HitTargets;
		ShotGun->ShotgunTraceWithScatter(HitTarget, HitTargets);
		if (!OwnerCharacter->HasAuthority()) ShotgunLocalFire(HitTargets);
		ServerShotgunFire(HitTargets);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (OwnerCharacter && CombatState == ECombatState::ECS_Unoccupied)
	{
		OwnerCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotGun* Shotgun = Cast<AShotGun>(EquippedWeapon);
	if (Shotgun == nullptr || OwnerCharacter == nullptr) return;
	if (CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Reloading)
	{
		OwnerCharacter->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
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

void UCombatComponent::ThrowGrenade()
{
	if (CarriedGrenade <= 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_ThrowGrenade;
	if (OwnerCharacter)
	{
		OwnerCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (OwnerCharacter && !OwnerCharacter->HasAuthority())
	{
		ServerThrowGrenade();
	}
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		CarriedGrenade = FMath::Clamp(CarriedGrenade - 1, 0, MaxGrenade);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::OnRep_Aiming()
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (CarriedGrenade <= 0) return;
	CombatState = ECombatState::ECS_ThrowGrenade;
	if (OwnerCharacter)
	{
		OwnerCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	CarriedGrenade = FMath::Clamp(CarriedGrenade - 1, 0, MaxGrenade);
	UpdateHUDGrenades();
}


void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDGrenades(CarriedGrenade);
	}
}


void UCombatComponent::ShotgunShellReload()
{
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::OnRep_CarriedCurrentWeaponAmmo()
{
	OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
	if (OwnerController)
	{
		OwnerController->SetHUDCarriedAmmo(CarriedCurrentWeaponAmmo);
	}
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun && (EquippedWeapon->IsAmmoFull() || CarriedCurrentWeaponAmmo <= 0);
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_ShotGun, StartingShotGunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
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
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (!EquippedWeapon->AmmoRunOut() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun)
	{
		return true;
	}
	if (bLocallyReload) return false;
	return !EquippedWeapon->AmmoRunOut() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	MulticastFire(TraceTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && !OwnerCharacter->HasAuthority()) return;
	LocalFire(TraceTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	MulticastShotgunFire_Implementation(TraceHitTargets);
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && !OwnerCharacter->HasAuthority()) return;
	ShotgunLocalFire(TraceHitTargets);
}

void UCombatComponent::SetAiming(bool InbAiming)
{
	if (OwnerCharacter == nullptr || EquippedWeapon == nullptr) return;
	bAiming = InbAiming;
	ServerSetAiming(InbAiming);
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (OwnerCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		OwnerCharacter->ShowSniperScopeWidget(bAiming);
	}
	if (OwnerCharacter->IsLocallyControlled()) bAimButtonPressed = InbAiming;
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
