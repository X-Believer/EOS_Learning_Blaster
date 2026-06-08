// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/TeamsGameMode.h"
#include "Game/BlasterGameState.h"
#include "Player/BlasterPlayerController.h"
#include "Player/BlasterPlayerState.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		ABlasterPlayerState* Player = NewPlayer->GetPlayerState<ABlasterPlayerState>();
		if (Player && Player->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BlasterGameState->BlueTeam.Num() <= BlasterGameState->RedTeam.Num())
			{
				Player->SetTeam(ETeam::ET_BlueTeam);
				BlasterGameState->BlueTeam.AddUnique(Player);
			}
			else
			{
				Player->SetTeam(ETeam::ET_RedTeam);
				BlasterGameState->RedTeam.AddUnique(Player);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* Player = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (BlasterGameState && Player)
	{
		if (BlasterGameState->RedTeam.Contains(Player))
		{
			BlasterGameState->RedTeam.Remove(Player);
		}
		else if (BlasterGameState->BlueTeam.Contains(Player))
		{
			BlasterGameState->BlueTeam.Remove(Player);
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		for (auto PlayerState : BlasterGameState->PlayerArray)
		{
			ABlasterPlayerState* Player = Cast<ABlasterPlayerState>(PlayerState);
			if (Player && Player->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->BlueTeam.Num() <= BlasterGameState->RedTeam.Num())
				{
					Player->SetTeam(ETeam::ET_BlueTeam);
					BlasterGameState->BlueTeam.AddUnique(Player);
				}
				else
				{
					Player->SetTeam(ETeam::ET_RedTeam);
					BlasterGameState->RedTeam.AddUnique(Player);
				}
			}
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return BaseDamage;
	if (AttackerPlayerState == VictimPlayerState) return BaseDamage;
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return 0.f;
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* KillerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, KillerController);
	
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* KillerPlayerState = KillerController ? Cast<ABlasterPlayerState>(KillerController->PlayerState) : nullptr;
	if (BlasterGameState && KillerPlayerState)
	{
		if (KillerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
		else if (KillerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
	}
}
