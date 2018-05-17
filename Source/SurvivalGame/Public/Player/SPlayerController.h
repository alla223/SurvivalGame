// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "SPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	FTimerHandle TimerHandle_Respawn;

	void OnKilled();

	void Respawn();


};
