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
}

void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{
	if (OwnerCharacter == nullptr || InWeapon == nullptr) return;
	
	EquippedWeapon = InWeapon;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	if (const USkeletalMeshSocket* RightHandSocket = OwnerCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		RightHandSocket->AttachActor(EquippedWeapon, OwnerCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(OwnerCharacter);
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
	}
}

void UCombatComponent::Fire()
{
	if (EquippedWeapon && bCanFire)
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
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	MulticastFire(TraceTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (OwnerCharacter)
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