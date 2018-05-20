// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivalGame.h"
#include "SWeapon.h"
#include "SCharacter.h"
#include "SPlayerController.h"
#include "STypes.h"
#include "Net/UnrealNetwork.h"

ASWeapon::ASWeapon(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
	Mesh = PCIP.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh3P"));
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh->bReceivesDecals = true;
	Mesh->CastShadow = true;
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = Mesh;

	//기본으로 Hand에 무기 장착할 거니까 true로 셋팅 추후 인벤토리 시스템 추가하면 그 때 false로 설정해서 착용 시 true로 바꾸던지 하겠다.
	bIsEquipped = true;
	CurrentState = EWeaponState::Idle;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	SetReplicates(true);
	bNetUseOwnerRelevancy = true;

	MuzzleAttachPoint = TEXT("MuzzleFlashSocket");
	StorageSlot = EInventorySlot::Hands;

	ShotsPerMinute = 700;
	StartAmmo = 999;
	MaxAmmo = 999;
	MaxAmmoPerClip = 30;
	NoAnimReloadDuration = 1.5f;
	NoEquipAnimDuration = 0.5f;
}

//simpactEffect override
void ASWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	/* Setup configuration */
	//시간(초)당 발사 수 
	TimeBetweenShots = 60.0f / ShotsPerMinute;
	CurrentAmmo = FMath::Min(StartAmmo, MaxAmmo);
	CurrentAmmoInClip = FMath::Min(MaxAmmoPerClip, StartAmmo);
}




/*
	Return Mesh of Weapon
*/
USkeletalMeshComponent* ASWeapon::GetWeaponMesh() const
{
	return Mesh;
}


class ASCharacter* ASWeapon::GetPawnOwner() const
{
	return MyPawn;
}


void ASWeapon::SetOwningPawn(ASCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// Net owner for RPC calls.
		SetOwner(NewOwner);
	}
}


void ASWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory(MyPawn);
	}
	//else
	//{
	//	OnLeaveInventory();

	//}
}


//현재 폰에 메시를 붙인다.
void ASWeapon::AttachMeshToPawn(EInventorySlot Slot)
{
	if (MyPawn)
	{
		USkeletalMeshComponent* PawnMesh = MyPawn->GetMesh();
		//현재 캐릭터의 Hand 의 FName 얻어온다.
		FName AttachPoint = MyPawn->GetInventoryAttachPoint(Slot);
		Mesh->SetHiddenInGame(false);
		//폰메쉬에 무기 붙인다.
		Mesh->AttachToComponent(PawnMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
	}
}


void ASWeapon::OnEnterInventory(ASCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
	AttachMeshToPawn(StorageSlot);
}


void ASWeapon::StartFire()
{
	//클라이언트 버전에서 serverRPC 호출한다. 서버에서 실행되고 현재 실행중인 서버에서 엔진 인스턴스가 담당하니까 서버가 role_Auturioty
	//클라 버전에서는 반대로 보인다.
	//아래 ServerStartFire()는 클라에서는 현재 Role이 Role_Authority보다 낮기 때문에 ServerStartFire를 호출할수 있다. 클라에서 호출했으니
	//서버에서 실행되고 클라이언트로 복제된다 .클라의 RemoteRole은 Role_Authority다.
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		//NewState = EWeaponState::Firing 로 설정
		DetermineWeaponState();
	}
}


void ASWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}


bool ASWeapon::ServerStartFire_Validate()
{
	return true;
}


void ASWeapon::ServerStartFire_Implementation()
{
	StartFire();
}


bool ASWeapon::ServerStopFire_Validate()
{
	return true;
}


void ASWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

//Pawn이 존재하고 무기상태가 idel이거나 firing이고 재장전중이아니면 true 반환
bool ASWeapon::CanFire() const
{
	bool bPawnCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOK = CurrentState == EWeaponState::Idle || CurrentState == EWeaponState::Firing;
	return bPawnCanFire && bStateOK && !bPendingReload;
}


FVector ASWeapon::GetAdjustedAim() const
{
	ASPlayerController* const PC = Instigator ? Cast<ASPlayerController>(Instigator->Controller) : nullptr;
	FVector FinalAim = FVector::ZeroVector;

	if (PC)
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}


FVector ASWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	ASPlayerController* PC = MyPawn ? Cast<ASPlayerController>(MyPawn->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		FRotator DummyRot;
		PC->GetPlayerViewPoint(OutStartTrace, DummyRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * (FVector::DotProduct((Instigator->GetActorLocation() - OutStartTrace), AimDir));
	}

	return OutStartTrace;
}


FHitResult ASWeapon::WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	FCollisionQueryParams TraceParams(TEXT("WeaponTrace"), true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, COLLISION_WEAPON, TraceParams);

	return Hit;
}



void ASWeapon::HandleFiring()
{
	if (CurrentAmmoInClip > 0 && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled()) 
		{
			FireWeapon();

			UseAmmo();

			// Update firing FX on remote clients if this is called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
		}

		/* Reload after firing last round */
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		/* Stop weapon fire FX, but stay in firing state */
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}
	//이것을 소유한 pawn에서 
	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		/* Retrigger HandleFiring on a delay for automatic weapons */
		bRefiring = (CurrentState == EWeaponState::Firing && TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ASWeapon::HandleFiring, TimeBetweenShots, false);
		}
	}


	//shot 한 이후의 시간을 추적
	LastFireTime = GetWorld()->GetTimeSeconds();
}

//이펙트/ 발사 애님/ 발사 사운드
void ASWeapon::SimulateWeaponFire()
{
	if (MuzzleFX)
	{
		MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh, MuzzleAttachPoint);	
	}

	if (!bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	PlayWeaponSound(FireSound);
}

//애님 중지
void ASWeapon::StopSimulatingWeaponFire()
{
	if (bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}
}



void ASWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}



bool ASWeapon::ServerHandleFiring_Validate()
{
	return true;
}


void ASWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		UseAmmo();

		// Update firing FX on remote clients
		BurstCounter++;
	}
}


void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWeapon, MyPawn);

	DOREPLIFETIME_CONDITION(ASWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASWeapon, CurrentAmmoInClip, COND_OwnerOnly);
	//프로퍼티 복제하면서 BurstCounter 동기화하고 HandleFire 다룬다.
	DOREPLIFETIME_CONDITION(ASWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASWeapon, bPendingReload, COND_SkipOwner);
}


FVector ASWeapon::GetMuzzleLocation() const
{
	return Mesh->GetSocketLocation(MuzzleAttachPoint);
}


FVector ASWeapon::GetMuzzleDirection() const
{
	return Mesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}


UAudioComponent* ASWeapon::PlayWeaponSound(USoundCue* SoundToPlay)
{
	UAudioComponent* AC = nullptr;
	if (SoundToPlay && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(SoundToPlay, MyPawn->GetRootComponent());
	}

	return AC;
}


EWeaponState ASWeapon::GetCurrentState() const
{
	return CurrentState;
}


void ASWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}


void ASWeapon::OnBurstStarted()
{
	// Start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && TimeBetweenShots > 0.0f &&
		LastFireTime + TimeBetweenShots > GameTime)
	{
		// &ASWeapon::HandleFiring을 (LastFireTime + TimeBetweenShots - GameTime)= TimeBetweenShots ,즉 TimeBetweenShost에 호출 
		//마지막 인자는 firstDelay로 처음에 마우스 홀드다운했을 때 발사까지의 시간이다.여기서는 설정 안하므로 누르자마자 발사된다.
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ASWeapon::HandleFiring, LastFireTime + TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}


void ASWeapon::OnBurstFinished()
{
	BurstCounter = 0;

	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}


void ASWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload())
			{
				NewState = EWeaponState::Reloading;
			}
			else
			{
				NewState = CurrentState;
			}
		}
		else if (!bPendingReload && bWantsToFire && CanFire())
		{
			NewState = EWeaponState::Firing;
		}
	}


	SetWeaponState(NewState);
}





float ASWeapon::PlayWeaponAnimation(UAnimMontage* Animation, float InPlayRate, FName StartSectionName)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		if (Animation)
		{
			Duration = MyPawn->PlayAnimMontage(Animation, InPlayRate, StartSectionName);
		}
	}

	return Duration;
}


void ASWeapon::StopWeaponAnimation(UAnimMontage* Animation)
{
	if (MyPawn)
	{
		if (Animation)
		{
			MyPawn->StopAnimMontage(Animation);
		}
	}
}

//??------------------------------------------------------------------------------------------




void ASWeapon::UseAmmo()
{
	CurrentAmmoInClip--;
	CurrentAmmo--;
}


int32 ASWeapon::GiveAmmo(int32 AddAmount)
{
	//필요한 ammo 개수 구한다  e.g) 30-14 = 16
	const int32 MissingAmmo = FMath::Max(0, MaxAmmo - CurrentAmmo);
	//충전량 
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	/* Push reload request to client */
	if (GetCurrentAmmoInClip() <= 0 && CanReload() &&
		MyPawn->GetCurrentWeapon() == this)
	{
		ClientStartReload();
	}

	/* Return the unused ammo when weapon is filled up */
	return FMath::Max(0, AddAmount - MissingAmmo);
}


void ASWeapon::SetAmmoCount(int32 NewTotalAmount)
{
	CurrentAmmo = FMath::Min(MaxAmmo, NewTotalAmount);
	CurrentAmmoInClip = FMath::Min(MaxAmmoPerClip, CurrentAmmo);
}


int32 ASWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}


int32 ASWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}


int32 ASWeapon::GetMaxAmmoPerClip() const
{
	return MaxAmmoPerClip;
}


int32 ASWeapon::GetMaxAmmo() const
{
	return MaxAmmo;
}


void ASWeapon::StartReload(bool bFromReplication)
{
	/* Push the request to server */
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	/* If local execute requested or we are running on the server */
	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &ASWeapon::StopSimulateReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &ASWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}


void ASWeapon::StopSimulateReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}


void ASWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(MaxAmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}
}


bool ASWeapon::CanReload()
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = (CurrentAmmoInClip < MaxAmmoPerClip) && ((CurrentAmmo - CurrentAmmoInClip) > 0);
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return (bCanReload && bGotAmmo && bStateOKToReload);
}


void ASWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		/* By passing true we do not push back to server and execute it locally */
		StartReload(true);
	}
	else
	{
		StopSimulateReload();
	}
}


void ASWeapon::ServerStartReload_Implementation()
{
	StartReload();
}


bool ASWeapon::ServerStartReload_Validate()
{
	return true;
}


void ASWeapon::ServerStopReload_Implementation()
{
	StopSimulateReload();
}


bool ASWeapon::ServerStopReload_Validate()
{
	return true;
}

//서버에서 호출되어 클라에서 실행
void ASWeapon::ClientStartReload_Implementation()
{
	StartReload();
}