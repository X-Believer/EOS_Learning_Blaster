// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/TeamsGameMode.h"
#include "CaptureFlagGameMode.generated.h"

class AFlag;
class AFlagZone;
/**
 * 
 */
UCLASS()
class BLASTER_API ACaptureFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* KillerController) override;
	void FlagCaptured(AFlag* Flag, AFlagZone* Zone);
};
