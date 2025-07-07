// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActor/NAPickableItemActor.h"
#include "NAPowerNode.generated.h"

UCLASS(Abstract)
class ARPG_API ANAPowerNode : public ANAPickableItemActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANAPowerNode(const FObjectInitializer& ObjectInitializer);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

//======================================================================================================================
// Item Use Interface Implements
//======================================================================================================================
public:
	virtual bool CanUseItem(UNAItemData* InItemData, AActor* User) const override;
	virtual bool UseItem(UNAItemData* InItemData, AActor* User, int32& UsedAmount) const override;
};
