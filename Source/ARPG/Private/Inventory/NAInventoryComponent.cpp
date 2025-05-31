// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/NAInventoryComponent.h"

#include "Inventory/GameInstance/NAInventoryGameInstanceSubsystem.h"
#include "Item/ItemActor/NAItemActor.h"
#include "Inventory/Widget/NAInventoryWidget.h"


UNAInventoryGameInstanceSubsystem* GetInvSubsys(const UNAInventoryComponent* InvComp)
{
	UNAInventoryGameInstanceSubsystem* InvSubsys = nullptr;
	if (InvComp)
	{
		if (UWorld* World = InvComp->GetWorld()) {
			if (UGameInstance* GI = World->GetGameInstance()) {
				InvSubsys = GI->GetSubsystem<UNAInventoryGameInstanceSubsystem>();
			}
		}
	}
	return InvSubsys;
}

// Sets default values for this component's properties
UNAInventoryComponent::UNAInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<UNAInventoryWidget> InventoryWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/00_ProjectNA/Inventory/BP_NAInventoryWidget.BP_NAInventoryWidget_C'"));
	check(InventoryWidgetClass.Class);
	SetWidgetClass(InventoryWidgetClass.Class);
}


// Called when the game starts
void UNAInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	SetVisibility(false);
}

FItemAddResult UNAInventoryComponent::HandleNonStackableItems(UNAItemData* InItem, int32 RequestedAddAmount)
{
	return {};
}

int32 UNAInventoryComponent::HandleStatckableItems(UNAItemData* InItem, int32 RequestedAddAmount)
{
	return 0;
}

int32 UNAInventoryComponent::CalculateWeightAddAmount(UNAItemData* InItem, int32 RequestedAddAmount)
{
	return {};
}

int32 UNAInventoryComponent::CalculateNumberForFullStack(UNAItemData* ExistingItem, int32 InitialRequestedAddAmount)
{
	return {};
}

// Called every frame
void UNAInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

UNAItemData* UNAInventoryComponent::FindMatchingItem(UNAItemData* InItem) const
{
	return nullptr;
}

UNAItemData* UNAInventoryComponent::FindNextItemByID(UNAItemData* InItem) const
{
	return nullptr;
}

UNAItemData* UNAInventoryComponent::FindNextPartialStack(UNAItemData* InItem) const
{
	return nullptr;
}

void UNAInventoryComponent::HandleAddNewItem(UNAItemData* Item, const int32 AmountToAdd)
{
	UNAInventoryGameInstanceSubsystem* InveSubsys = GetInvSubsys(this);
	if (InveSubsys)
	{
		InveSubsys->AddItemToInventory(Item->GetItemID(), Item->Quantity);
	}
}


void UNAInventoryComponent::HandleRemoveSingleItemActor(UNAItemData* InItem)
{
	UNAInventoryGameInstanceSubsystem* InveSubsys = GetInvSubsys(this);
	if (InveSubsys)
	{
		InveSubsys->RemoveItemFromInventory(InItem->GetItemID(), 1);
	}
}

int32 UNAInventoryComponent::HandleRemoveAmountOfItemActors(UNAItemData* InItem, int32 DesiredAmountToRemove)
{
	UNAInventoryGameInstanceSubsystem* InveSubsys = GetInvSubsys(this);
	int32 RemovedAmount = 0;
	if (InveSubsys)
	{
		RemovedAmount = InveSubsys->RemoveItemFromInventory(InItem->GetItemID(), DesiredAmountToRemove);
	}
	return RemovedAmount;
}

void UNAInventoryComponent::SplitExistingStack(UNAItemData* InItem, const int32 AmountToSplit)
{
}

void UNAInventoryComponent::ReleaseInventoryWidget()
{
	if (UUserWidget* InventoryWidget = GetWidget())
	{
		SetVisibility(true);
		SetWindowVisibility(EWindowVisibility::Visible);
		InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UNAInventoryComponent::CollapseInventoryWidget()
{
	if (UUserWidget* InventoryWidget = GetWidget())
	{
		SetVisibility(false);
		SetWindowVisibility(EWindowVisibility::SelfHitTestInvisible);
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

