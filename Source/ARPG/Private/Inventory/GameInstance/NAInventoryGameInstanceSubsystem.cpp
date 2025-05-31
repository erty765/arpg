// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/GameInstance/NAInventoryGameInstanceSubsystem.h"

#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"

void UNAInventoryGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 아이템 서브 시스템의 Initialize가 먼저 호출되도록 보장
	check(UNAItemEngineSubsystem::Get());
	
	Super::Initialize(Collection);

	if (GEngine)
	{
		bool bInventoryInitialized = false;
		// @TODO: 세이브 파일에서 인벤토리 데이터를 읽어와서 Inventory를 초기화하는 로직

		// Inventory를 순회하며 InventoryItems를 초기화
		if (bInventoryInitialized)
		{
			for (const FNAInventorySlot& InvSlot : Inventory)
			{
				const UNAItemData* NewItemData = UNAItemEngineSubsystem::Get()->CreateItemDataBySlot(GetWorld(), InvSlot);
				if (!NewItemData)
				{
					UE_LOG(LogTemp, Warning, TEXT("[UNAInventoryGameInstanceSubsystem::Initialize]  인벤토리 슬롯으로 아이템 데이터 생성 실패"));
					continue;
				}
				InventoryItems.Emplace(NewItemData->GetItemID(), InvSlot.ItemSlotStack);
			}
		}
	}
	else
	{
		ensureAlwaysMsgf(false, TEXT("[UNAInventoryGameInstanceSubsystem::Initialize]  아이템 서브시스템의 초기화에 문제가 생김... 아마도"));
	}
}

void UNAInventoryGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UNAInventoryGameInstanceSubsystem::AddItemToInventory(const FName& ItemID, int32 Stack)
{
	if (UNAItemData* InputItem = UNAItemEngineSubsystem::Get()->GetRuntimeItemData(ItemID))
	{
		const bool bResult = AddItemToInventory(InputItem, Stack);
		return bResult;
	}
	
	return false;
}

bool UNAInventoryGameInstanceSubsystem::AddItemToInventory(UNAItemData* InItem, int32 Stack)
{
	if (InItem)
	{
		if (GEngine)
		{
			UClass* ItemClass = InItem->GetClass();
			UNAItemData* OldItem = nullptr;
			int32 OldStack = 0;
			for (TPair<FName,int32>& Pair : InventoryItems)
			{
				if (OldItem == UNAItemEngineSubsystem::Get()->GetRuntimeItemData(Pair.Key))
				{
					if (ItemClass == OldItem->GetItemActorClass())
					{
						OldStack = Pair.Value;
						break;
					}
				}
			}

			if (OldItem)
			// case 1) 새로운 아이템과 똑같은 클래스의 아이템이 있는지 확인
			//		있다? → 두 아이템 데이터의 state가 동일한가?
			//					→ yes: 해당 아이템이 있는 슬롯을 찾고, 찾은 슬롯에 아이템 수량을 추가. 
			//							자동 병합 시스템? (인벤토리 안) 원래 아이템의 데이터에 새로운 아이템 데이터의 수량을 합침. 새로운 아이템 데이터는 제거(이 후처리에 대해서는 좀 더 고민할 필요)
			//					→ no: 동일한 클래스의 아이템이 있는 슬롯의 근처에서 빈 슬롯을 찾는다. 찾은 빈 슬롯에 새로운 아이템 데이터를 할당한다.
			{
				if (OldItem->ItemState == InItem->ItemState)
				{
					
				}
				else
				{
					
				}
			}
			// case 2) 아예 새로운 아이템
			else
			{
		
			}
		}
	}
	
	return false;
}

int32 UNAInventoryGameInstanceSubsystem::RemoveItemFromInventory(const FName& ItemID, int32 Stack)
{
	int32 RemovedItems = 0;
	if (InventoryItems.Contains(ItemID))
	{
		if (InventoryItems[ItemID] <= 0)
		{
			// 뭐임 왜 인벤토리에 있는 아이템인데 스택이 0임??
		}
		
		if (InventoryItems[ItemID] - Stack == 0)
		{
			// 다 삭제 TMap::Remove
		}
		else if (InventoryItems[ItemID] - Stack < 0)
		{
			// 오류? 삭제가 요청된 수량이 인벤토리가 보유한 수량보다 컸음
		}
		else if (InventoryItems[ItemID] - Stack > 0)
		{
			// 인벤토리 상 슬롯은 냅두고, 수량만 줄이기
		}
	}
	return false;
}

bool UNAInventoryGameInstanceSubsystem::EquipItem(const FName& ItemMetaID)
{
	return false;
}

bool UNAInventoryGameInstanceSubsystem::UnequipItem(const FName& ItemMetaID)
{
	return false;
}

