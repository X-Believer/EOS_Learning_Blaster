// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/BuffComponent.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	if (HealingTime > 0.f)
	{
		HealingRate = HealAmount / HealingTime;
		AmountToHeal += HealAmount;
	}
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bReplenishingShield = true;
	if (ReplenishTime > 0.f)
	{
		ReplenishRate = ShieldAmount / ReplenishTime;
		AmountToReplenish += ShieldAmount;
	}
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (OwnerCharacter == nullptr) return;
	OwnerCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeed, BuffTime);
	
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		OwnerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::SetInitialSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::ResetSpeed()
{
	if (OwnerCharacter == nullptr || OwnerCharacter->GetCharacterMovement() == nullptr) return;
	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (OwnerCharacter == nullptr) return;
	OwnerCharacter->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &UBuffComponent::ResetJump, BuffTime);
	
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::ResetJump()
{
	if (OwnerCharacter == nullptr || OwnerCharacter->GetCharacterMovement() == nullptr) return;
	OwnerCharacter->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	}
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || OwnerCharacter == nullptr || OwnerCharacter->IsEliminated()) return;
	
	const float HealThisFrame = HealingRate * DeltaTime;
	OwnerCharacter->SetHealth(FMath::Clamp(OwnerCharacter->GetHealth() + HealThisFrame, 0.f, OwnerCharacter->GetMaxHealth()));
	OwnerCharacter->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;
	
	if (AmountToHeal <= 0.f || OwnerCharacter->GetHealth() >= OwnerCharacter->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bReplenishingShield || OwnerCharacter == nullptr || OwnerCharacter->IsEliminated()) return;
	
	const float ShieldThisFrame = ReplenishRate * DeltaTime;
	OwnerCharacter->SetShield(FMath::Clamp(OwnerCharacter->GetShield() + ShieldThisFrame, 0.f, OwnerCharacter->GetMaxShield()));
	OwnerCharacter->UpdateHUDShield();
	AmountToReplenish -= ShieldThisFrame;
	
	if (AmountToReplenish <= 0.f || OwnerCharacter->GetShield() >= OwnerCharacter->GetMaxShield())
	{
		bReplenishingShield = false;
		AmountToReplenish = 0.f;
	}
}
