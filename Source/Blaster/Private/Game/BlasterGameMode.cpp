// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BlasterPlayerController.h"
#include "Player/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* KillerController)
{
	ABlasterPlayerState* KillerPlayerState = KillerController ? Cast<ABlasterPlayerState>(KillerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	
	if (KillerPlayerState && VictimPlayerState != KillerPlayerState)
	{
		KillerPlayerState->AddToScore(1.f);
	}
	
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeatNum(1);
	}
	
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}
	if (EliminatedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
	}
}
