// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/CaptureFlagGameMode.h"

#include "BlasterTypes/Team.h"
#include "CaptureTheFlag/FlagZone.h"
#include "Game/BlasterGameState.h"
#include "Weapon/Flag.h"

void ACaptureFlagGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,ABlasterPlayerController* VictimController, ABlasterPlayerController* KillerController)
{
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, KillerController);
	
}

void ACaptureFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
	if (BlasterGameState)
	{
		if (Zone->Team == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
		else if (Zone->Team == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
	}
}
