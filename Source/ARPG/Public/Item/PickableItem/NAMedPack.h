// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActor/NAPickableItemActor.h"
#include "Item/ItemDataStructs/NARecoveryPackDataStructs.h"
#include "NAMedPack.generated.h"

UCLASS(Abstract)
class ARPG_API ANAMedPack : public ANAPickableItemActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANAMedPack(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	//virtual void BeginInteract_Implementation(AActor* Interactor) override;
	//virtual bool ExecuteInteract_Implementation(AActor* Interactor) override;
	//virtual void EndInteract_Implementation(AActor* Interactor) override;

	
//======================================================================================================================
// Item Use Interface Implements
//======================================================================================================================
public:
	virtual bool CanUseItem(UNAItemData* InItemData, AActor* User) const override;
	virtual bool UseItem(UNAItemData* InItemData, AActor* User, int32& UsedAmount) const override;

	EMedPackGrade GetMedPackGrade() const;
};
