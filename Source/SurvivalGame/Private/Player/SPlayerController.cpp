// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SPlayerController.h"
#include "SGameMode.h"
#include "GameFramework/Actor.h"

void ASPlayerController::OnKilled() {
	//�� ������ ���� �ʴ´�.
	UnPossess(); //F12 �� playercontroller Ȯ�� -> Pawn�� �����ϰ� ���� unposses�Ѵ�.
	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &ASPlayerController::Respawn, 2.f);
}

void ASPlayerController::Respawn()
{

	ASGameMode *GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		//���� �ڸ����� ����������
		//APawn * NewPawn = GM->SpawnDefaultPawnFor(this, StartSpot.Get());
		//������ ��Ʈ�ѷ�(this)�� ���� �ڸ����� �����ϱ� ���� �ڵ� ��������. 0514
		APawn * NewPawn = GM->SpawnDefaultPawnFor(this, StartSpot.Get());
		Possess(NewPawn);
	}
}
