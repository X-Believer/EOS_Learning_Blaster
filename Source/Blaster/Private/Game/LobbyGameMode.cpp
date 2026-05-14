// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	int32 NumOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumOfPlayers == 2)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
		}
	}
}
