// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SPlayerController.h"
#include "SGameMode.h"
#include "SPlayerCameraManager.h"
#include "GameFramework/Actor.h"


ASPlayerController::ASPlayerController(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* Assign the class types we wish to use */
	PlayerCameraManagerClass = ASPlayerCameraManager::StaticClass();

}

