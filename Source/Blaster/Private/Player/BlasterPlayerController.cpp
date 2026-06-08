// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/WidgetAnimation.h"
#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Game/BlasterGameMode.h"
#include "Game/BlasterGameState.h"
#include "GameFramework/GameMode.h"
#include "HUD/Announcement.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/ReturnToMainMenu.h"
#include "Input/BlasterInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerState.h"

ABlasterPlayerController::ABlasterPlayerController()
{
	bReplicates = true;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime >= TimeSyncFrequency)
	{		
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::BroadCastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
			}
			else if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
			}
			else if (Attacker == Self && Victim == Self )
			{
				BlasterHUD->AddElimAnnouncement("You", "Yourself");
			}
			else
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			}
		}
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
	
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
	BlasterInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABlasterPlayerController::Reload);
	BlasterInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Started, this, &ABlasterPlayerController::ThrowGrenade);
	BlasterInputComponent->BindAction(QuitAction, ETriggerEvent::Started, this, &ABlasterPlayerController::ShowReturnToMainMenu);
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	OwnerCharacter = Cast<ABlasterCharacter>(InPawn);
	SetHUDHealth(OwnerCharacter->GetHealth(), OwnerCharacter->GetMaxHealth());
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else
	{
		SetHUDAnnouncementCountdown(0.f);
		SetHUDMatchCountdown(0.f);
		return;
	}
	
	uint32 SecondLeft = FMath::CeilToInt(TimeLeft);
	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode.Get();
		if (BlasterGameMode)
		{
			SecondLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	
	if (CountdownInt != SecondLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}
	if (BlasterHUD)
	{
		if (CharacterOverlay == nullptr)
		{
			if (BlasterHUD && BlasterHUD->CharacterOverlay)
			{
				CharacterOverlay = BlasterHUD->CharacterOverlay;
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);				
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeatNum) SetHUDDefeatNum(HUDDefeatNum);
				if (bInitializeGrenades) SetHUDGrenades(HUDGrenades);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
	
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombatComponent())
				{
					SetHUDGrenades(BlasterCharacter->GetCombatComponent()->GetCarriedGrenade());
				}
			}
		}
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HighPingImage && BlasterHUD->CharacterOverlay->HighPingAnim;
	
	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->PlayAnimation(
			BlasterHUD->CharacterOverlay->HighPingAnim,
			0.f,
			5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HighPingImage && BlasterHUD->CharacterOverlay->HighPingAnim;
	
	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnim))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnim);
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime >= CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState.Get();
		if (PlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ping: %d"), PlayerState->GetCompressedPing() * 4);
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	
	if (BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HighPingAnim && BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnim))
	{
		PingAnimRunningTime += DeltaTime;
		if (PingAnimRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

// Is the ping to high
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (BlasterGameMode)
	{
		WarmupTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		CooldownTime = BlasterGameMode->CooldownTime;
		LevelStartingTime = BlasterGameMode->LevelStartingTime;
		MatchState = BlasterGameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedRequest)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = RoundTripTime / 2.f;
	const float CurrentServerTime = TimeServerReceivedRequest + SingleTripTime;
	ServerClientDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
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
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->ShieldBar && BlasterHUD->CharacterOverlay->ShieldText;
	
	if (bHUDValid)
	{
		const float HealthPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(HealthPercent);
		FString ShieldText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
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
	else
	{
		bInitializeDefeatNum = true;
		HUDDefeatNum = DefeatNum;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->AmmoNum;
	
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->AmmoNum->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->CarriedAmmoNum;
	
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoNum->SetText(FText::FromString(CarriedAmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->MatchCountdownText;
	
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountDownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->Announcement && BlasterHUD->Announcement->WarmupTime;
	
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountDownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	bool bHUDValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->GrenadesNum;
	
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesNum->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())		
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ServerClientDelta;
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::Move(const FInputActionValue& Value)
{
	if (OwnerCharacter && OwnerCharacter->bDisableGameplay) return;
	
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
		if (OwnerCharacter->bDisableGameplay) return;
		OwnerCharacter->Jump();
	}
}

void ABlasterPlayerController::Equip(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		
		OwnerCharacter->EquipWeapon();
	}
}

void ABlasterPlayerController::Crouch(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		
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
		if (OwnerCharacter->bDisableGameplay) return;
		
		OwnerCharacter->AimBegin();
	}
}

void ABlasterPlayerController::AimEnd(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		OwnerCharacter->AimEnd();
	}
}

void ABlasterPlayerController::FireBegin(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		OwnerCharacter->FireBegin();
	}
}

void ABlasterPlayerController::FireEnd(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		OwnerCharacter->FireEnd();
	}
}

void ABlasterPlayerController::Reload(const FInputActionValue& Value)
{
	if (OwnerCharacter)
	{
		if (OwnerCharacter->bDisableGameplay) return;
		OwnerCharacter->Reload();
	}
}

void ABlasterPlayerController::ThrowGrenade(const FInputActionValue& Value)
{
	if (OwnerCharacter->GetCombatComponent())
	{
		OwnerCharacter->ThrowGrenade();
	}
}

void ABlasterPlayerController::ShowReturnToMainMenu(const FInputActionValue& Value)
{
	if (ReturnToMainMenuWidgetClass == nullptr) return;
	if (ReturnToMainMenuWidget == nullptr)
	{
		ReturnToMainMenuWidget = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidgetClass);
	}
	if (ReturnToMainMenuWidget)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenuWidget->MenuSetup();
		}
		else
		{
			ReturnToMainMenuWidget->MenuTeardown();
		}
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingToStart)
	{
		HandleWaitingToStart();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingToStart)
	{
		HandleWaitingToStart();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleWaitingToStart()
{
	if (BlasterHUD && !BlasterHUD->Announcement)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD.Get();
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		if (BlasterHUD->Announcement && BlasterHUD->Announcement->AnnouncementText && BlasterHUD->Announcement->InfoText)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString Announcement("New Match Starts In:");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(Announcement));
			
			const ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			const ABlasterPlayerState* LocalPlayerState = GetPlayerState<ABlasterPlayerState>();
			
			if (BlasterGameState && LocalPlayerState)
			{
				TArray<ABlasterPlayerState*> TopScoringPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoText;
				if (TopScoringPlayers.Num() == 0)
				{
					InfoText = FString("No winner");
				}
				else if (TopScoringPlayers.Num() == 1 && TopScoringPlayers[0] == LocalPlayerState)
				{
					InfoText = FString("You won!");
				}
				else if (TopScoringPlayers.Num() == 1 && TopScoringPlayers[0] != LocalPlayerState)
				{
					InfoText = FString::Printf(TEXT("%s won!"), *TopScoringPlayers[0]->GetPlayerName());
				}
				else if (TopScoringPlayers.Num() > 1)
				{
					InfoText = FString("Tie between:\n");
					for (const ABlasterPlayerState* TopScoringPlayer : TopScoringPlayers)
					{						
						InfoText.Append(FString::Printf(TEXT("%s\n"), *TopScoringPlayer->GetPlayerName()));
					}
					InfoText.RemoveFromEnd(TEXT("\n"));
				}
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoText));
			}
		}
	}
	if (OwnerCharacter && OwnerCharacter->GetCombatComponent())
	{
		OwnerCharacter->bDisableGameplay = true;
		OwnerCharacter->GetCombatComponent()->FirePressed(false);
	}
}
