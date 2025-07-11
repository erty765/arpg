#include "Item/ItemData/NAItemData.h"

#include "AbilitySystemInterface.h"
#include "Item/ItemActor/NAItemActor.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"

#include "Inventory/Component/NAInventoryComponent.h"
#include "Item/NAItemUseInterface.h"

// 프로그램 시작 시 0 에서 시작
FThreadSafeCounter UNAItemData::IDCount(0);

UNAItemData::UNAItemData()
{
	if (!HasAnyFlags(RF_ClassDefaultObject)) {
		IDNumber = IDCount.Increment();
	}
	ID = NAME_None;
}

void UNAItemData::PostInitProperties()
{
	Super::PostInitProperties();
}

void UNAItemData::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity != Quantity)
	{
		if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct<FNAItemBaseTableRow>()) {
			Quantity = FMath::Clamp(NewQuantity, 0,
				ItemMetaData->NumericData.bIsStackable ? ItemMetaData->NumericData.MaxSlotStackSize : 1);
		}
	}
}

void UNAItemData::SetItemState(EItemState NewItemState)
{
	if (ItemState != NewItemState)
	{
		ItemState = NewItemState;
		// 델리게이트?
	}
}

EItemType UNAItemData::GetItemType() const
{
	if (const FNAItemBaseTableRow* ItemDataStruct = GetItemMetaDataStruct())
	{
		return ItemDataStruct->ItemType;
	}
	return EItemType::IT_None;
}

UClass* UNAItemData::GetItemActorClass() const {
	if (const FNAItemBaseTableRow* ItemMetaData
		= GetItemMetaDataStruct()) {
		return ItemMetaData->ItemClass.Get();
	}
	return nullptr;
}

FString UNAItemData::GetItemName() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->TextData.Name.ToString();
	}
	return {};
}

FText UNAItemData::GetItemDescription() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->TextData.Description;
	}
	return FText::GetEmpty();
}

UTexture2D* UNAItemData::GetItemIcon() const {
	if (const FNAItemBaseTableRow* ItemMetaData
		= GetItemMetaDataStruct()) {
		return ItemMetaData->IconAssetData.ItemIcon;
	}
	return nullptr;
}

bool UNAItemData::IsPickableItem() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->ItemType != EItemType::IT_None && ItemMetaData->ItemType != EItemType::IT_Misc;
	}
	return false;
}

bool UNAItemData::IsStackableItem() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->NumericData.bIsStackable;
	}
	return false;
}

int32 UNAItemData::GetItemMaxSlotStackSize() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->NumericData.MaxSlotStackSize;
	}
	return -1;
}

int32 UNAItemData::GetMaxInventoryHoldCount() const
{
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		return ItemMetaData->NumericData.MaxInventoryHoldCount;
	}
	return -1;
}

void UNAItemData::SetOwningInventory(UNAInventoryComponent* NewInventory)
{
	if (NewInventory != nullptr)
	{
		OwningInventory = NewInventory;
	}
}

bool UNAItemData::TryUseItem(AActor* User)
{
	UClass* ItemClass = GetItemActorClass();
	if (!ItemClass) return false;

	UObject* CDO = ItemClass->GetDefaultObject(false);
	if (!CDO) return false;

	bool bSucceed = false;
	if (INAItemUseInterface* ItemUseInterface = Cast<INAItemUseInterface>(CDO)) {
		if (!ItemUseInterface->CanUseItem(this, User)) return false;
		
		int32 UsedAmount = 0;
		bSucceed = ItemUseInterface->UseItem(this, User, UsedAmount);
		if (bSucceed && UsedAmount > 0) {
			int32 PredictedQuantity = Quantity - UsedAmount;
		
			if (OwningInventory.IsValid()) {
				// 수량/상태 변경 시 인벤토리 위젯 갱신 요청
				// 수량이 0이면 아이템 데이터 제거까지 처리
				return OwningInventory->TryRemoveItem(ID, UsedAmount);
			}
			
			if (PredictedQuantity <= 0) {
				// 아이템 수량이 0 이하일 때, 아이템 엔진 서브시스템에서 데이터와 액터를 완전히 제거
				if (UNAItemEngineSubsystem::Get())
				{
					return UNAItemEngineSubsystem::Get()
					->DestroyRuntimeItemData(ID, true);
				}
			}
			else {
				SetQuantity(PredictedQuantity);
			}
		}
	}
	return bSucceed;
}

bool UNAItemData::GetInteractableData(FNAInteractableData& OutData) const
{
	if (!ID.IsNone())
	{
		if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
		{
			OutData = ItemMetaData->InteractableData;
			return true;
		}
	}
	return false;
}

bool UNAItemData::IsCurrencyItem() const
{
	return GetItemType() == EItemType::IT_UpgradeNode
		|| GetItemType() == EItemType::IT_Credit;
}
