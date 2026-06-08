// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterPlayerState;

namespace MatchState
{
	extern BLASTER_API const FName Cooldown;
}

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ABlasterGameMode();
	virtual void Tick( float DeltaTime ) override;
	virtual void PlayerEliminated(ABlasterCharacter	*EliminatedCharacter, ABlasterPlayerController *VictimController, ABlasterPlayerController *KillerController);
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);
	virtual float CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage);
	
	UFUNCTION()
	void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);
	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	UPROPERTY()
	float LevelStartingTime = 0.f;
	
	bool bTeamMatch = false;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
private:
	float CountdownTime = 0.f;
	
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
