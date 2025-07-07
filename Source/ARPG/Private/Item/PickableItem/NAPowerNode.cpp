// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PickableItem/NAPowerNode.h"


// Sets default values
ANAPowerNode::ANAPowerNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PickupMode = EPickupMode::PM_Inventory;
}

// Called when the game starts or when spawned
void ANAPowerNode::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasValidItemID())
	{
		GetItemData()->SetQuantity(1);
	}
}

// Called every frame
void ANAPowerNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ANAPowerNode::CanUseItem(UNAItemData* InItemData, AActor* User) const
{
	return Super::CanUseItem(InItemData, User);
}

bool ANAPowerNode::UseItem(UNAItemData* InItemData, AActor* User, int32& UsedAmount) const
{
	return Super::UseItem(InItemData, User, UsedAmount);
}


