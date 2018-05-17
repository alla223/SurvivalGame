// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SGameMode.h"
#include "SPlayerController.h"
#include "SGameState.h"
#include "SCharacter.h"



ASGameMode::ASGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/PlayerPawn"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;


	PlayerControllerClass = ASPlayerController::StaticClass();
}
