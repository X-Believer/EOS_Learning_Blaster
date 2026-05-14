// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"

#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(const APawn* InPawn)
{
	const ENetRole LocalRole = InPawn->GetLocalRole();
	FString RoleString;
	switch (LocalRole)
	{
		case ENetRole::ROLE_Authority:
			RoleString = "Authority";
			break;
		case ENetRole::ROLE_AutonomousProxy:
			RoleString = "AutonomousProxy";
			break;
		case ENetRole::ROLE_SimulatedProxy:
			RoleString = "SimulatedProxy";
			break;
		case ENetRole::ROLE_None:
			RoleString = "None";
			break;
		default:
			RoleString = "Unknown";
			break;
	}
	const FString LocalRoleString = FString::Printf(TEXT(""));
	
	SetDisplayText(LocalRoleString);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
