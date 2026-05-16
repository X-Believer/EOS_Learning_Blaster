// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Score() override;
	UFUNCTION()
	virtual void OnRep_DefeatNum();
	void AddToScore(float ScoreAmount);
	void AddToDefeatNum(int32 InDefeatNum);
	
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	
	UPROPERTY(ReplicatedUsing=OnRep_DefeatNum)
	int32 DefeatNum;
};
