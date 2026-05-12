// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"
#include "Input/BlasterInputComponent.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(BlasterMappingContext);
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(BlasterMappingContext, 0);
	}
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UBlasterInputComponent* BlasterInputComponent = CastChecked<UBlasterInputComponent>(InputComponent);
	
	BlasterInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterPlayerController::Move);
	BlasterInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterPlayerController::Look);
	BlasterInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterPlayerController::Jump);
}

void ABlasterPlayerController::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.X == 0.0f && MovementVector.Y == 0.0f) return;
	
	const FRotator ControllerRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControllerRotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABlasterPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddPitchInput(LookAxisVector.Y);
	AddYawInput(LookAxisVector.X);
}
void ABlasterPlayerController::Jump(const FInputActionValue& Value)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->Jump();
	}
}
