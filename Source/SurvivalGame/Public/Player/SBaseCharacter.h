// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "STypes.h"
#include "SBaseCharacter.generated.h"

USTRUCT()
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

		//가해지는 데미지
		UPROPERTY()
		float ActualDamage;

	//데미지 타입 e.g_) 화염 뎀지 유형정의 -> 일정 유형의 함수 주고 -> takeDamage 호출
	UPROPERTY()
		UClass* DamageTypeClass;

	//누가 데미지를 입혔나 . 플레이어가 되고
	UPROPERTY()
		TWeakObjectPtr<class ASBaseCharacter> PawnInstigator;

	// 무엇이 데미지를 입혔나. e.g) 방금지나간 액터 ,point hole에서는 다른 플레이어가 된다. 
	UPROPERTY()
		TWeakObjectPtr<class AActor> DamageCauser;

	//point 데미지인가 radial 인가 구분 
	UPROPERTY()
		uint8 DamageEventClassID;

	//플레이어가 죽었나 체크
	UPROPERTY()
		bool bKilled;

private:

	UPROPERTY()
		uint8 EnsureReplicationByte;

	UPROPERTY()
		FDamageEvent GeneralDamageEvent;

	UPROPERTY()
		FPointDamageEvent PointDamageEvent;

	// 히트 데이터를 가지고  클라에게 복제해 히트와 죽은것 보냄 
public:
	FTakeHitInfo()
		: ActualDamage(0),
		DamageTypeClass(nullptr),
		PawnInstigator(nullptr),
		DamageCauser(nullptr),
		DamageEventClassID(0),
		bKilled(false),
		EnsureReplicationByte(0)
	{}

	FDamageEvent& GetDamageEvent()
	{
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			if (PointDamageEvent.DamageTypeClass == nullptr)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;

		default:
			if (GeneralDamageEvent.DamageTypeClass == nullptr)
			{
				GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return GeneralDamageEvent;
		}
	}


	void SetDamageEvent(const FDamageEvent& DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
			break;
		default:
			GeneralDamageEvent = DamageEvent;
		}
	}


	void EnsureReplication()
	{
		EnsureReplicationByte++;
	}
};




UCLASS(ABSTRACT)
class SURVIVALGAME_API ASBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	ASBaseCharacter(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
		USoundCue* SoundTakeHit;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
		USoundCue* SoundDeath;


	/************************************************************************/
	/* Health                                                               */
	/************************************************************************/


	UFUNCTION(BlueprintCallable, Category = "PlayerCondition")
		float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "PlayerCondition")
		float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "PlayerCondition")
		bool IsAlive() const;


	//애님 블프 이벤트그래프에서 셋팅해준다.
	UFUNCTION(BlueprintCallable, Category = "Movement")
		virtual bool IsSprinting() const;

	/* Client/local call to update sprint state  */
	virtual void SetSprinting(bool NewSprinting);

	float GetSprintingSpeedModifier() const;
protected:


	UPROPERTY(EditDefaultsOnly, Category = "Movement")
		float SprintingSpeedModifier;

	/* Character wants to run, checked during Tick to see if allowed */
	UPROPERTY(Transient, Replicated)
		bool bWantsToRun;

	/* Server side call to update actual sprint state */
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetSprinting(bool NewSprinting);

	void ServerSetSprinting_Implementation(bool NewSprinting);

	bool ServerSetSprinting_Validate(bool NewSprinting);

	/************************************************************************/
	/* Targeting                                                            */
	/************************************************************************/

	void SetTargeting(bool NewTargeting);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetTargeting(bool NewTargeting);

	void ServerSetTargeting_Implementation(bool NewTargeting);

	bool ServerSetTargeting_Validate(bool NewTargeting);

	UPROPERTY(Transient, Replicated)
		bool bIsTargeting;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
		float TargetingSpeedModifier;
public:

	/* Is player aiming down sights */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
		bool IsTargeting() const;

	float GetTargetingSpeedModifier() const;

	/* Retrieve Pitch/Yaw from current camera */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
		FRotator GetAimOffsets() const;


	/************************************************************************/
	/* Damage & Death                                                       */
	/************************************************************************/

protected:

	UPROPERTY(EditDefaultsOnly, Category = "PlayerCondition", Replicated)
		float Health;

	/* Take damage & handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	virtual bool Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser);

	virtual void OnDeath(float KillingDamage, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);

	virtual void FellOutOfWorld(const class UDamageType& DmgType) override;

	void SetRagdollPhysics();

	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled);

	void ReplicateHit(float DamageTaken, struct FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled);

	/* Holds hit data to replicate hits and death to clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
		struct FTakeHitInfo LastTakeHitInfo;

	UFUNCTION()
		void OnRep_LastTakeHitInfo();

	bool bIsDying;

};
