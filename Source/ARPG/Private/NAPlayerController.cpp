// Fill out your copyright notice in the Description page of Project Settings.


#include "ARPG/Public/NAPlayerController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NAGameStateBase.h"
#include "ARPG/NAGlobalDelegate.h"
#include "Combat/PhysicsHandleComponent/NAKineticComponent.h"
#include "Item/PlaceableItem/Door/NADoor.h"
#include "Lobby/NASessionListWidget.h"


ANAPlayerController::ANAPlayerController()
{
}

void ANAPlayerController::Server_VoteForRestart_Implementation( APlayerState* InPlayerState, const bool bValue )
{
	if ( ANAGameStateBase* GameStateBase = Cast<ANAGameStateBase>( GetWorld()->GetGameState() ) )
	{
		GameStateBase->VoteForRestart( InPlayerState, bValue );
	}
}

bool ANAPlayerController::Server_VoteForRestart_Validate( APlayerState* InPlayerState, const bool bValue )
{
	if ( const ANAGameStateBase* GameStateBase = Cast<ANAGameStateBase>( GetWorld()->GetGameState() ) )
	{
		return GameStateBase->PlayerArray.Find( InPlayerState ) && GameStateBase->IsFailed();
	}

	return false;
}

void ANAPlayerController::Multi_RemoveFailedWidget_Implementation()
{
	if ( ANAGameStateBase* GameStateBase = Cast<ANAGameStateBase>( GetWorld()->GetGameState() ) )
	{
		GameStateBase->RemoveFailedWidget();
	}
}

void ANAPlayerController::Server_ToggleDoor_Implementation( ANADoor* Door )
{
	Multi_ToggleDoor( Door );
}

void ANAPlayerController::Multi_ToggleDoor_Implementation( ANADoor* Door )
{
	Door->ToggleDoor();
}

void ANAPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ANAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// if (InteractionComponent)
	// {
	// 	AttachToPawn(InPawn);
	// }
}

void ANAPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if ( UNAKineticComponent* KineticComponent = P->GetComponentByClass<UNAKineticComponent>() )
	{
		KineticComponent->SetOwningController( this );
	}
}

void ANAPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if ( GetNetMode() == NM_Client )
	{
		// 클라이언트 방향 PlayerState Replication 추적
		GOnNewPlayerStateChanged.Broadcast( GetPlayerState<APlayerState>() );
	}
}

void ANAPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	// if (InteractionComponent)
	// {
	// 	DetachFromPawn();
	// }
}
