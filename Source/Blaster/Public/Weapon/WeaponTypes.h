// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define TRACE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252


UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_SMG UMETA(DisplayName = "Submachine Gun"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_ShotGun UMETA(DisplayName = "ShotGun"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_SniperRifle	UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher	UMETA(DisplayName = "Grenade Launcher"),
	
	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};