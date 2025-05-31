#include "Item/ItemData/NAItemData.h"
#include "Item/ItemActor/NAItemActor.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "Inventory/GameInstance/NAInventoryGameInstanceSubsystem.h"

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

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UNAItemData::PostInitProperties] CDO) %s"), *GetName());
		
		if (ID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[UNAItemData::PostInitProperties] CDO) %s's ID is none."),*GetName());
		}
	}
	else if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UNAItemData::PostInitProperties]  일반) %s"), *GetName());
		if (ID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[UNAItemData::PostInitProperties]  일반) %s's ID is none."),*GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[UNAItemData::PostInitProperties]  일반) %s's ID: %s."), *GetName(), *ID.ToString());
			// 예상) 레벨에 배치된 아이템 액터가 에디터 켜질때 처음 로드되는 경우: UNAItemEngineSubsystem의 CreateItemDataByActor를 호출하지 않음
		}
		if (GetOuter() != UNAItemEngineSubsystem::Get())
		{
			UE_LOG(LogTemp, Warning,
				   TEXT(
					   "[UNAItemData::PostInitProperties]  일반) %s's Outer[%s] is NOT UNAItemEngineSubsystem. 억덕계 이런 일이"
				   ), *GetName(), *GetOuter()->GetName());
		}
	}
}

void UNAItemData::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity != Quantity)
	{
		if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct<FNAItemBaseTableRow>()) {
			Quantity = FMath::Clamp(NewQuantity, 0, ItemMetaData->NumericData.bIsStackable ? ItemMetaData->NumericData.MaxSlotStackSize : 1);
			/*if (OwningInventory.Get())
			{
				if (Quantity <= 0)
				{
					OwningInventory->RemoveItem(this);
				}
			}*/
		}
	}
}

UClass* UNAItemData::GetItemActorClass() const
{
	UClass* ItemActorClass = nullptr;
	if (const FNAItemBaseTableRow* ItemMetaData = GetItemMetaDataStruct())
	{
		ItemActorClass = ItemMetaData->ItemClass.Get();
	}
	return ItemActorClass;
}
