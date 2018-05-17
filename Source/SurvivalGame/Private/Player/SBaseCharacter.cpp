// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SurvivalGame.h"
#include "SBaseCharacter.h"
#include "SGameMode.h"
#include "SCharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ASBaseCharacter::ASBaseCharacter(const class FObjectInitializer& ObjectInitializer)
	/* Override the movement class from the base class to our own to support multiple speeds (eg. sprinting) */
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Health = 100;

	TargetingSpeedModifier = 0.5f;
	SprintingSpeedModifier = 2.0f;

	/* Don't collide with camera checks to keep 3rd person camera at position when zombies or other players are standing behind us */
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}


float ASBaseCharacter::GetHealth() const
{
	return Health;
}


float ASBaseCharacter::GetMaxHealth() const
{
	// Retrieve the default value of the health property that is assigned on instantiation.
	return GetClass()->GetDefaultObject<ASBaseCharacter>()->Health;
}


bool ASBaseCharacter::IsAlive() const
{
	return Health > 0;
}


void ASBaseCharacter::SetSprinting(bool NewSprinting)
{
	bWantsToRun = NewSprinting;

	if (bIsCrouched)
	{
		UnCrouch();
	}

	if (Role < ROLE_Authority)
	{
		ServerSetSprinting(NewSprinting);
	}
}


void ASBaseCharacter::ServerSetSprinting_Implementation(bool NewSprinting)
{
	SetSprinting(NewSprinting);
}


bool ASBaseCharacter::ServerSetSprinting_Validate(bool NewSprinting)
{
	return true;
}


bool ASBaseCharacter::IsSprinting() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return bWantsToRun && !IsTargeting() && !GetVelocity().IsZero()
		// Don't allow sprint while strafing sideways or standing still (1.0 is straight forward, -1.0 is backward while near 0 is sideways or standing still)
		&& (FVector::DotProduct(GetVelocity().GetSafeNormal2D(), GetActorRotation().Vector()) > 0.8); // Changing this value to 0.1 allows for diagonal sprinting. (holding W+A or W+D keys)
}


float ASBaseCharacter::GetSprintingSpeedModifier() const
{
	return SprintingSpeedModifier;
}



void ASBaseCharacter::SetTargeting(bool NewTargeting)
{
	bIsTargeting = NewTargeting;

	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(NewTargeting);
	}
}


void ASBaseCharacter::ServerSetTargeting_Implementation(bool NewTargeting)
{
	SetTargeting(NewTargeting);
}


bool ASBaseCharacter::ServerSetTargeting_Validate(bool NewTargeting)
{
	return true;
}



bool ASBaseCharacter::IsTargeting() const
{
	return bIsTargeting;
}


float ASBaseCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}


FRotator ASBaseCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

void ASBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Value is already updated locally, skip in replication step
	DOREPLIFETIME_CONDITION(ASBaseCharacter, bWantsToRun, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASBaseCharacter, bIsTargeting, COND_SkipOwner);

	// Replicate to every client, no special condition required
	DOREPLIFETIME(ASBaseCharacter, Health);

}
