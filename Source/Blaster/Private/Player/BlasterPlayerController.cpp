// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Input/BlasterInputComponent.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	
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
	BlasterInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABlasterPlayerController::Jump);
	BlasterInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ABlasterPlayerController::Equip);
	BlasterInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterPlayerController::Crouch);
	BlasterInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterPlayerController::AimBegin);
	BlasterInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterPlayerController::AimEnd);
	BlasterInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterPlayerController::FireBegin);
	BlasterInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterPlayerController::FireEnd);
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	OwnerCharacter = Cast<ABlasterCharacter>(InPawn);
	SetHUDHealth(OwnerCharacter->GetHealth(), OwnerCharacter->GetMaxHealth());
}

void ABlasterPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	OwnerCharacter = Cast<ABlasterCharacter>(GetPawn());
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText;
	
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->ScoreNum;
	
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreNum->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeatNum(int32 DefeatNum)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->DefeatNum;
	
	if (bHUDValid)
	{
		FString DefeatText = FString::Printf(TEXT("%d"), DefeatNum);
		BlasterHUD->CharacterOverlay->DefeatNum->SetText(FText::FromString(DefeatText));
	}
}

void ABlasterPlayerController::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.X == 0.0f && MovementVector.Y == 0.0f) return;
	
	const FRotator ControllerRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControllerRotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	if (OwnerCharacter)
	{
		OwnerCharacter->AddMovementInput(ForwardDirection, MovementVector.Y);
		OwnerCharacter->AddMovementInput(RightDirection, MovementVector.X);
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
	if (OwnerCharacter)
	{
		OwnerCharacter->Jump();
	}
}

void ABlasterPlayerController::Equip(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->EquipWeapon();
	}
}

void ABlasterPlayerController::Crouch(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bIsCrouched)
		{
			OwnerCharacter->UnCrouch();
		}
		else
		{
			OwnerCharacter->Crouch();
		}
	}
}

void ABlasterPlayerController::AimBegin(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->AimBegin();
	}
}

void ABlasterPlayerController::AimEnd(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->AimEnd();
	}
}

void ABlasterPlayerController::FireBegin(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->FireBegin();
	}
}

void ABlasterPlayerController::FireEnd(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->FireEnd();
	}
}
