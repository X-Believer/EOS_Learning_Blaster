// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"


void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABlasterPlayerState, DefeatNum);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter.Get();
	if (BlasterCharacter)
	{		
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterPlayerController.Get();
		if (BlasterPlayerController)
		{
			BlasterPlayerController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter.Get();
	if (BlasterCharacter)
	{		
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterPlayerController.Get();
		if (BlasterPlayerController)
		{
			BlasterPlayerController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_DefeatNum()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter.Get();
	if (BlasterCharacter)
	{		
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterPlayerController.Get();
		if (BlasterPlayerController)
		{
			BlasterPlayerController->SetHUDDefeatNum(DefeatNum);
		}
	}
}

void ABlasterPlayerState::AddToDefeatNum(int32 InDefeatNum)
{
	DefeatNum += InDefeatNum;
	
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter.Get();
	if (BlasterCharacter)
	{		
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterPlayerController.Get();
		if (BlasterPlayerController)
		{
			BlasterPlayerController->SetHUDDefeatNum(DefeatNum);
		}
	}
}

