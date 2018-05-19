// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SBaseCharacter.h"
#include "STypes.h"
#include "SCharacter.generated.h"

UCLASS()
class SURVIVALGAME_API ASCharacter : public ASBaseCharacter
{
	GENERATED_BODY()

	ASCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	/* Called every frame */
	virtual void Tick(float DeltaSeconds) override;

	/* Called to bind functionality to input */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	/* Stop playing all montages */
	void StopAllAnimMontages();



private:

	/* Boom to handle distance to player mesh. */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoomComp;

	/* Primary camera of the player*/
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* CameraComp;


public:

	

	FORCEINLINE UCameraComponent* GetCameraComponent()
	{
		return CameraComp;
	}

	
	/************************************************************************/
	/* Movement                                                             */
	/************************************************************************/

	virtual void MoveForward(float Val);

	virtual void MoveRight(float Val);

	/* Client mapped to Input */
	void OnCrouchToggle();

	/* Client mapped to Input */
	void OnJump();

	/* Client mapped to Input */
	void OnStartSprinting();

	/* Client mapped to Input */
	void OnStopSprinting();

	virtual void SetSprinting(bool NewSprinting) override;

	/* Is character currently performing a jump action. Resets on landed.  */
	UPROPERTY(Transient, Replicated)
	bool bIsJumping;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsInitiatedJump() const;

	void SetIsJumping(bool NewJumping);

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerSetIsJumping(bool NewJumping);

	void ServerSetIsJumping_Implementation(bool NewJumping);

	bool ServerSetIsJumping_Validate(bool NewJumping);

	/** 엔진 내 코드 Falling under the effects of gravity, such as after jumping or walking off the edge of a surface. -> EmovementMode Falling 설명*/
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;


	/************************************************************************/
	/* Targeting                                                            */
	/************************************************************************/

	void OnStartTargeting();

	void OnEndTargeting();

	


	/************************************************************************/
	/* Weapons & Inventory                                                  */
	/************************************************************************/

private:

	/* Attachpoint for active weapon/item in hands */
	UPROPERTY(EditDefaultsOnly, Category = "Sockets")
	FName WeaponAttachPoint;

	bool bWantsToFire;


	void OnReload();

	/* Mapped to input */
	void OnStartFire();

	/* Mapped to input */
	void OnStopFire();

	void StartWeaponFire();

	void StopWeaponFire();


public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
		ASWeapon* GetCurrentWeapon() const;

	/* Check if pawn is allowed to fire weapon */
	bool CanFire() const;

	bool CanReload() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsFiring() const;


	/*어태치먼트하기위한 소켓이름 반환. 슬롯같은것들 바꾸지 않을거라 const Return socket name for attachments (to match the socket in the character skeleton) */
	FName GetInventoryAttachPoint(EInventorySlot Slot) const;

	//복제함.
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon)
		class ASWeapon* CurrentWeapon;

	/* OnRep functions can use a parameter to hold the previous value of the variable. Very useful when you need to handle UnEquip etc. */
	UFUNCTION()
		void OnRep_CurrentWeapon(ASWeapon* LastWeapon);


	void SetCurrentWeapon(class ASWeapon* newWeapon);

	void EquipWeapon(ASWeapon* Weapon);

	UFUNCTION(Reliable, Server, WithValidation)
		void ServerEquipWeapon(ASWeapon* Weapon);

	void ServerEquipWeapon_Implementation(ASWeapon* Weapon);

	bool ServerEquipWeapon_Validate(ASWeapon* Weapon);

	void AddWeapon(class ASWeapon* Weapon);

	/* All weapons/items the player currently holds */
	UPROPERTY(Transient, Replicated)
		TArray<ASWeapon*> Inventory;
};
