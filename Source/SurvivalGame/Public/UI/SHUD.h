// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SHUD.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVALGAME_API ASHUD : public AHUD
{
	GENERATED_BODY()
	
		ASHUD(const FObjectInitializer& ObjectInitializer);


	/** Main HUD update loop. */
	virtual void DrawHUD() override;
	
	
};
