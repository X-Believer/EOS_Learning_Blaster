// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Weapon/Casing.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
	else
	{
		OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
		if (OwnerCharacter && OwnerCharacter->GetEquippedWeapon() && OwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			
			if (UWorld* World = GetWorld())
			{
				World->SpawnActor<ACasing>(
					CasingClass, 
					SocketTransform.GetLocation(), 
					SocketTransform.GetRotation().Rotator());
			}
		}
	}
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachmentRules);
	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget) const
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket == nullptr) return FVector();
	const FTransform MuzzleFlashSocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = MuzzleFlashSocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;
	
	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, false, 2.f);
	// DrawDebugLine(GetWorld(), TraceStart, EndLoc, FColor::Blue, false, 2.f);
	// DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Blue, false, 2.f);
	
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::SetHUDAmmo()
{
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
	if (OwnerCharacter)
	{
		OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
		if (OwnerController)
		{
			OwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		Sequence++;
	}
}

void AWeapon::AddAmmo(int32 AmmoAmount)
{
	Ammo = FMath::Clamp(Ammo + AmmoAmount, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoAmount);
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	Sequence--;
	Ammo = FMath::Clamp(Ammo - Sequence, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::ClientAddAmmo_Implementation(int32 InAmmo)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + InAmmo, 0, MagCapacity);
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
	if (OwnerCharacter && OwnerCharacter->GetCombatComponent() && IsAmmoFull())
	{
		OwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeapon::SetWeaponState(const EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	default:
		break;
	}
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	default:
		break;
	}
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnableCustomDepth(false);
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
		if (OwnerController && HasAuthority() && !OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	if (GetWeaponMesh())
	{
		GetWeaponMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		GetWeaponMesh()->MarkRenderStateDirty();
	}
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
	
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter.Get();
	if (OwnerCharacter && bUseServerSideRewind)
	{
		OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->GetController()) : OwnerController.Get();
		if (OwnerController && HasAuthority() && OwnerController->HighPingDelegate.IsBound())
		{
			OwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

bool AWeapon::AmmoRunOut() const
{
	return Ammo <= 0;
}

bool AWeapon::IsAmmoFull() const
{
	return Ammo >= MagCapacity;
}

void AWeapon::ShowPickupWidget(const bool bShowWidget) const
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}
