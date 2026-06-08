// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Player/BlasterPlayerState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() > TopScoringPlayers[0]->GetScore())
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScoringPlayers[0]->GetScore())
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::RedTeamScores()
{
	RedTeamScore++;
	
	ABlasterPlayerController* Controller = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (Controller)
	{
		Controller->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::BlueTeamScores()
{
	BlueTeamScore++;
	
	ABlasterPlayerController* Controller = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (Controller)
	{
		Controller->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	ABlasterPlayerController* Controller = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (Controller)
	{
		Controller->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	ABlasterPlayerController* Controller = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (Controller)
	{
		Controller->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
