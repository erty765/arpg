#include "Item/ItemDataStructs/NAItemBaseDataStructs.h"
#include "Item/PickableItem/NAWeapon.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "Item/ItemActor/NAPlaceableItemActor.h"
#include "Item/PickableItem/NAPowerNode.h"
#include "Misc/StringUtils.h"

#if WITH_EDITOR
void FNAItemBaseTableRow::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	FNAItemBaseTableRow* ItemRowStruct = InDataTable->FindRow<FNAItemBaseTableRow>(InRowName, TEXT("On Data Table Changed"));
	
	if (ItemRowStruct == this)
	{
		if (!ItemRowStruct->ItemClass.IsValid()) return;
		
		if (UNAItemEngineSubsystem::Get()
			&& UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized())
		{
			UClass* ItemActorClass = ItemRowStruct->ItemClass.Get();
			if (ItemActorClass)
			{
				if (!UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(ItemActorClass))
				{
					UNAItemEngineSubsystem::Get()->RegisterNewItemMetaData(ItemActorClass, InDataTable, InRowName);
				}
				else
				{
					UNAItemEngineSubsystem::Get()->VerifyItemMetaDataRowHandle(ItemActorClass, InDataTable, InRowName);
				}
			}
		}

		if (ItemType == EItemType::IT_Weapon
			|| ItemClass.Get()->IsChildOf<ANAWeapon>())
		{
			NumericData.bIsStackable = false;
			NumericData.MaxSlotStackSize = 1;
			NumericData.MaxInventoryHoldCount = 1;
		}
		else
		{
			if (!NumericData.bIsStackable)
			{
				NumericData.MaxSlotStackSize = 1;
				NumericData.MaxInventoryHoldCount = FMath::Max(0, NumericData.MaxInventoryHoldCount);
			}
			
			if (NumericData.MaxInventoryHoldCount == 0)
			{
				NumericData.MaxSlotStackSize = FMath::Max(0, NumericData.MaxSlotStackSize);
			}
			else if (NumericData.MaxInventoryHoldCount > 0)
			{
				NumericData.MaxSlotStackSize = FMath::Max(1, NumericData.MaxSlotStackSize);
			}
		}
		
		if (!InRowName.IsNone())
		{
			FString NewItemName = FStringUtils::InsertSpacesBeforeUppercaseSmart(InRowName.ToString());
			TextData.Name = FText::FromString(NewItemName);
		}

		if (InteractableData.InteractableType != ENAInteractableType::None)
		{
			FString EnumStr = FStringUtils::EnumToDisplayString(InteractableData.InteractableType);
			EnumStr = FStringUtils::InsertSpacesBeforeUppercaseSmart(EnumStr);
			InteractableData.InteractionName = FText::FromString(EnumStr);
		}

		if (ItemClass.Get()->IsChildOf<ANAPlaceableItemActor>())
		{
			InteractableData.bIsUnlimitedInteractable = true;
			InteractableData.InteractableCount = 0;
		}

		if (ItemClass.Get()->IsChildOf<ANAPowerNode>())
		{
			ItemType = EItemType::IT_PowerNode;
		}

		if (ItemType == EItemType::IT_Credit || ItemType == EItemType::IT_PowerNode)
		{
			NumericData.bIsStackable = true;
			NumericData.MaxSlotStackSize = 0;
			NumericData.MaxInventoryHoldCount = 0;
			
			if (InteractableData.InteractableType == ENAInteractableType::None)
			{
				InteractableData.InteractableType = ENAInteractableType::Pickup;
			}
		}
	}
}
#endif