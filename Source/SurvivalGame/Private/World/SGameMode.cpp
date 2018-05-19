// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SGameMode.h"
#include "SPlayerController.h"
#include "SGameState.h"
#include "SWeapon.h"
#include "STypes.h"
#include "SCharacter.h"



ASGameMode::ASGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/PlayerPawn"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;


	PlayerControllerClass = ASPlayerController::StaticClass();

}

void ASGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	SpawnDefaultInventory(PlayerPawn);
}


void ASGameMode::SpawnDefaultInventory(APawn* PlayerPawn)
{
	ASCharacter* MyPawn = Cast<ASCharacter>(PlayerPawn);
	if (MyPawn)
	{
		for (int32 i = 0; i < DefaultInventoryClass.Num(); i++)
		{
			if (DefaultInventoryClass[i])
			{
				//액터 스폰
				FActorSpawnParameters SpawnInfo;
				SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ASWeapon* NewWeapon = GetWorld()->SpawnActor<ASWeapon>(DefaultInventoryClass[i], SpawnInfo);
				//
				MyPawn->AddWeapon(NewWeapon);
			}
		}
	}
}
