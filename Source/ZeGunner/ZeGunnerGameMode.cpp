// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZeGunnerGameMode.h"
#include "FighterPawn.h"
#include "FighterPlayerController.h"
#include "FighterHUD.h"

AZeGunnerGameMode::AZeGunnerGameMode()
{
	DefaultPawnClass = AFighterPawn::StaticClass();
	PlayerControllerClass = AFighterPlayerController::StaticClass();
	HUDClass = AFighterHUD::StaticClass();
}
