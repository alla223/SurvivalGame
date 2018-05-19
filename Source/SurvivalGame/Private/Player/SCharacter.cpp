// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SWeapon.h"
#include "SCharacter.h"
#include "SBaseCharacter.h"
#include "SCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "SPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Engine/Classes/Animation/AnimInstance.h"

// Sets default values
ASCharacter::ASCharacter(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	// Adjust jump to make it less floaty
	MoveComp->GravityScale = 1.5f;
	MoveComp->JumpZVelocity = 620;
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;
	MoveComp->MaxWalkSpeedCrouched = 200;

	/* Ignore this channel or it will absorb the trace impacts instead of the skeletal mesh */
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	// Enable crouching
	MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;

	CameraBoomComp = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
	CameraBoomComp->SocketOffset = FVector(0, 35, 0);
	CameraBoomComp->TargetOffset = FVector(0, 0, 55);
	CameraBoomComp->bUsePawnControlRotation = true;
	CameraBoomComp->SetupAttachment(GetRootComponent());

	CameraComp = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("Camera"));
	CameraComp->SetupAttachment(CameraBoomComp);


	TargetingSpeedModifier = 0.5f;
	SprintingSpeedModifier = 2.5f;

	Health = 100;


	/* Names as specified in the character skeleton */
	WeaponAttachPoint = TEXT("WeaponSocket");

	//
}


void ASCharacter::BeginPlay()
{
	Super::BeginPlay();


}


void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bWantsToRun && !IsSprinting())
	{
		SetSprinting(true);
	}

	
}



// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Movement
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("SprintHold", IE_Pressed, this, &ASCharacter::OnStartSprinting);
	PlayerInputComponent->BindAction("SprintHold", IE_Released, this, &ASCharacter::OnStopSprinting);

	PlayerInputComponent->BindAction("CrouchToggle", IE_Released, this, &ASCharacter::OnCrouchToggle);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::OnJump);

	// Weapons
	PlayerInputComponent->BindAction("Targeting", IE_Pressed, this, &ASCharacter::OnStartTargeting);
	PlayerInputComponent->BindAction("Targeting", IE_Released, this, &ASCharacter::OnEndTargeting);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::OnStartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::OnStopFire);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::OnReload);


}


void ASCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);

		AddMovementInput(Direction, Val);
	}
}


void ASCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FRotator Rotation = GetActorRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}





void ASCharacter::OnStartTargeting()
{

	SetTargeting(true);
}


void ASCharacter::OnEndTargeting()
{
	SetTargeting(false);
}


void ASCharacter::OnJump()
{
	SetIsJumping(true);
}


bool ASCharacter::IsInitiatedJump() const
{
	return bIsJumping;
}


void ASCharacter::SetIsJumping(bool NewJumping)
{
	// crocuh 상태에서 점프 매핑이 들어오면 크라우치를 해제한다.
	if (bIsCrouched && NewJumping)
	{
		UnCrouch();
	}
	//점프 매핑동작 시 참이 되는 변수와 / 캐릭터가 현재 점프동작을 하는지 체크 -> 서로 다르다면
	else if (NewJumping != bIsJumping)
	{
		bIsJumping = NewJumping;

		if (bIsJumping)
		{
			/* Perform the built-in Jump on the character */
			Jump();
		}
	}

	if (Role < ROLE_Authority)
	{
		ServerSetIsJumping(NewJumping);
	}
}

void ASCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	/* Check if we are no longer falling/jumping */
	if (PrevMovementMode == EMovementMode::MOVE_Falling &&
		GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Falling)
	{
		SetIsJumping(false);
	}
}




void ASCharacter::ServerSetIsJumping_Implementation(bool NewJumping)
{
	SetIsJumping(NewJumping);
}


bool ASCharacter::ServerSetIsJumping_Validate(bool NewJumping)
{
	return true;
}


void ASCharacter::OnStartSprinting()
{

	SetSprinting(true);
}


void ASCharacter::OnStopSprinting()
{
	SetSprinting(false);
}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Value is already updated locally, skip in replication step
	DOREPLIFETIME_CONDITION(ASCharacter, bIsJumping, COND_SkipOwner);

	// Replicate to every client, no special condition required
	DOREPLIFETIME(ASCharacter, Health);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, Inventory);
	/* If we did not display the current inventory on the player mesh we could optimize replication by using this replication condition. */
	/* DOREPLIFETIME_CONDITION(ASCharacter, Inventory, COND_OwnerOnly);*/
}

void ASCharacter::OnCrouchToggle()
{
	if (IsSprinting())
	{
		SetSprinting(false);
	}

	// If we are crouching then CanCrouch will return false. If we cannot crouch then calling Crouch() wont do anything
	if (CanCrouch())
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}


bool ASCharacter::CanFire() const
{
	/* Add your own checks here, for example non-shooting areas or checking if player is in an NPC dialogue etc. */
	return IsAlive();
}


bool ASCharacter::CanReload() const
{
	return IsAlive();
}

//문제가능성 있음.
bool ASCharacter::IsFiring() const
{
	return CurrentWeapon && CurrentWeapon->GetCurrentState() == EWeaponState::Firing;
}

// 어태치포인트 이름만 반환하는 함수 
FName ASCharacter::GetInventoryAttachPoint(EInventorySlot Slot) const
{
	/* Return the socket name for the specified storage slot */
	switch (Slot)
	{
	case EInventorySlot::Hands:
		return WeaponAttachPoint;
	default:
		// Not implemented.
		return "";
	}
}


void ASCharacter::SetCurrentWeapon(class ASWeapon* NewWeapon)
{
	CurrentWeapon = NewWeapon;

	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);
	}
}


void ASCharacter::OnRep_CurrentWeapon(ASWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon);
}


ASWeapon* ASCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}


void ASCharacter::EquipWeapon(ASWeapon* Weapon)
{
	if (Weapon)
	{

		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}


bool ASCharacter::ServerEquipWeapon_Validate(ASWeapon* Weapon)
{
	return true;
}


void ASCharacter::ServerEquipWeapon_Implementation(ASWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void ASCharacter::AddWeapon(class ASWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);

		// Equip first weapon in inventory
		if (Inventory.Num() > 0 && CurrentWeapon == nullptr)
		{
			EquipWeapon(Inventory[0]);
		}
	}
}



void ASCharacter::OnReload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

//입력 매핑 함수 
void ASCharacter::OnStartFire()
{
	//스프린팅 중이라면 스프린팅 해제한다
	if (IsSprinting())
	{
		SetSprinting(false);
	}

	//fire 함수 호출
	StartWeaponFire();
}


void ASCharacter::OnStopFire()
{
	StopWeaponFire();
}


void ASCharacter::StartWeaponFire()
{
	//Fire 제어 멤버변수 확인
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		//Weapon ->StartFire()
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}


void ASCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

void ASCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}




void ASCharacter::SetSprinting(bool NewSprinting)
{
	if (bWantsToRun)
	{
		StopWeaponFire();
	}

	Super::SetSprinting(NewSprinting);
}
