// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SPlayerController.h"
#include "SGameMode.h"
#include "GameFramework/Actor.h"

void ASPlayerController::OnKilled() {
	//폰 조종을 하지 않는다.
	UnPossess(); //F12 로 playercontroller 확인 -> Pawn을 복제하고 폰을 unposses한다.
	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &ASPlayerController::Respawn, 2.f);
}

void ASPlayerController::Respawn()
{

	ASGameMode *GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		//원래 자리에서 스폰하지만
		//APawn * NewPawn = GM->SpawnDefaultPawnFor(this, StartSpot.Get());
		//소유한 컨트롤러(this)를 랜덤 자리에서 스폰하기 위해 코드 수정했음. 0514
		APawn * NewPawn = GM->SpawnDefaultPawnFor(this, StartSpot.Get());
		Possess(NewPawn);
	}
}
