// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "SGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API ASGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ASGameMode(const FObjectInitializer& ObjectInitializer);

	/* The default weapons to spawn with */
	UPROPERTY(EditDefaultsOnly, Category = "Player")
		TArray<TSubclassOf<class ASWeapon>> DefaultInventoryClass;

	virtual void Killed(AController* Killer, AController* VictimPlayer, APawn* VictimPawn, const UDamageType* DamageType);

protected:

	/**
	* Make sure pawn properties are back to default
	* Also a good place to modify them on spawn
	*/
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	virtual void SpawnDefaultInventory(APawn* PlayerPawn);
};
