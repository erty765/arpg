// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NAPlayerController.generated.h"

class ANADoor;
class UNASessionListEntryData;
/**
 * 
 */
UCLASS()
class ARPG_API ANAPlayerController : public APlayerController
{
	
	GENERATED_BODY()

public:
	ANAPlayerController();

	UFUNCTION( Server, Unreliable, WithValidation )
	void Server_VoteForRestart( APlayerState* InPlayerState, const bool bValue );

	UFUNCTION( NetMulticast, Reliable )
	void Multi_RemoveFailedWidget();

	UFUNCTION( Server, Reliable )
	void Server_ToggleDoor( ANADoor* Door );

	UFUNCTION( NetMulticast, Reliable )
	void Multi_ToggleDoor( ANADoor* Door );

protected:
	virtual void BeginPlay() override;
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void AcknowledgePossession(APawn* P) override;

	virtual void OnRep_PlayerState() override;
	
	virtual void OnUnPossess() override;
};
