// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/GameInstance/NAItemGameInstanceSubsystem.h"

#include "Item/ItemActor/NAItemActor.h"
#include "Inventory/DataStructs/NAInventoryDataStructs.h"
#include "Item/ItemDataStructs/NAItemBaseDataStructs.h"
#include "Item/NAItemEngineSubsystem.h"

void FItemManagerImpl::Initialize()
{
	if (ItemDataTableSources.IsEmpty())
	{
		// 1) Registry 에셋 동기 로드 (나중에 실제 경로로 교체)
		static const FString RegistryPath = TEXT("/Script/ARPG.ItemDataTablesAsset'/Game/00_ProjectNA/ItemTest/DA_ItemDataTables.DA_ItemDataTables'");
		UItemDataTablesAsset* Registry = Cast<UItemDataTablesAsset>(StaticLoadObject(UItemDataTablesAsset::StaticClass(), nullptr, *RegistryPath));

		if (!Registry)
		{
			UE_LOG(LogTemp, Error, TEXT("[UNAItemGameInstanceSubsystem::Initialize]  ItemDataTablesAsset 로드 실패: %s"), *RegistryPath);
			return;
		}

		// 2) Registry 안의 SoftObjectPtr<UDataTable> 리스트 순회
		for (const TSoftObjectPtr<UDataTable>& SoftDT : Registry->ItemDataTables)
		{
			UDataTable* DT = SoftDT.LoadSynchronous();
			if (!DT)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[UNAItemGameInstanceSubsystem]  Failed to load DataTable: %s"), *SoftDT.ToString());
				continue;
			}

			ItemDataTableSources.Emplace(DT);
			UE_LOG(LogTemp, Log,
				TEXT("[UNAItemGameInstanceSubsystem]  Loaded DataTable: %s"), *DT->GetName());
		}

		// (2) 메타데이터 맵 빌드
		if (ItemMetaDataMap.IsEmpty())
		{
			// ItemMetaDataMap 버킷 확보
			int32 ExpectedCount = 0;
			for (UDataTable* DT : ItemDataTableSources)
			{
				ExpectedCount += DT->GetRowMap().Num();
			}
			ItemMetaDataMap.Reserve(ExpectedCount);
	
			for (UDataTable* DT : ItemDataTableSources)
			{
				for (const TPair<FName, uint8*>& Pair : DT->GetRowMap())
				{
					FName  RowName = Pair.Key;
					FNAItemBaseTableRow* Row = DT->FindRow<FNAItemBaseTableRow>(RowName, TEXT("Mapping item meta data"));
					if (Row && Row->ItemClass)
					{
						ValidateItemRow(Row, RowName);
				
						FDataTableRowHandle Handle;
						Handle.DataTable = DT;
						Handle.RowName = RowName;
						ItemMetaDataMap.Emplace(Row->ItemClass, Handle);
					}
				}
			}

			if (!ItemMetaDataMap.IsEmpty()) {
				bIsMetaMapInitialized = true;
			}
		}
	}

	// 블루프린트 (아이템) - 매시 지정, 아이템 효과 지정, 기타 등등 -> (Item 클래스)
	// 테이블에 모든 아이템을 넣어 놓고 -> ( 아이템을 저장할 테이블 Row, TSubClassof<Item클래스> + 아이템 설명 등등 (Ability 제외, GAS로 조회가능) )
	// 게임 인스턴스에서 테이블을 순회하면서 메타 데이터를 생성 -> ( 아이템 인스턴스에 Item 클래스, 아이템 설명 복사)

	// 레벨에 있는 아이템 = 그냥 있으면 됌 (얘내는 굳이 메타 데이터가 필요 없음 ㅇㅇ...)
	// 레벨에 있는 아이템의 설명 = 메타 데이터를 게임 인스턴스에서 조회하고 보여주면 됨 (아이템 -> 메타 데이터)
	// 인벤토리 = 메타 데이터로 변환 후 배열에 저장 (아이템 -> 메타 데이터)
	// 인벤토리 -> 드랍 = 메타 데이터로 아이템 클래스를 조회 후 SpawnActor (메타 데이터 -> 아이템)

}

void FItemManagerImpl::UpdateRuntimeItemDataMap( FName InItemDataID, const UNAItemData* InItemData ) const
{
	if (!InItemDataID.IsNone()) {
		ensureAlwaysMsgf(false, TEXT("[UNAItemGameInstanceSubsystem::UpdateRuntimeItemDataMap]  InItemDataID가 유효하지 않음."));
	}

	if (!InItemData) {
		ensureAlwaysMsgf(false, TEXT("[UNAItemGameInstanceSubsystem::UpdateRuntimeItemDataMap]  InItemData가 유효하지 않음."));
		return;
	}

	if (RuntimeItemDataMap.Contains(InItemDataID)) {
		ensureAlwaysMsgf(false, TEXT("[UNAItemGameInstanceSubsystem::UpdateRuntimeItemDataMap]  InItemDataID가 이미 존재함."));
		return;
	}
}

void FItemManagerImpl::ValidateItemRow( const FNAItemBaseTableRow* RowPtr, const FName RowName )
{
	const int32 Slot = RowPtr->NumericData.MaxSlotStackSize;
	const int32 Inv  = RowPtr->NumericData.MaxInventoryStackSize;
	if (Inv != 0 && Slot > Inv)
	{
		ensureMsgf(
			   false,
			   TEXT("DataTable(%s): 오류! MaxSlotStackSize(%d) > MaxInventoryStackSize(%d)"),
			   *RowName.ToString(), Slot, Inv);
		
	}
	else if (Inv != 0 && Slot == 0)
	{
		ensureMsgf(
			   false,
			   TEXT("DataTable(%s): 오류! MaxInventoryStackSize(%d)이 0보다 큰데, MaxSlotStackSize(%d)이 0 이었음"),
			   *RowName.ToString(), Slot, Inv);
	}
}

UNAItemData* FItemManagerImpl::TryCreateItemData(UWorld* OuterHandle)
{
	UNAItemData* NewItemData = nullptr;
	if (OuterHandle && GEngine)
	{
		if (UNAItemEngineSubsystem* ItemEngineSubsys = GEngine->GetEngineSubsystem<UNAItemEngineSubsystem>())
		if (OuterHandle->IsEditorWorld() || OuterHandle->WorldType == EWorldType::Inactive)
		{
			NewItemData = NewObject<UNAItemData>(ItemEngineSubsys, NAME_None, RF_DuplicateTransient);
		}
		else
		{
			NewItemData = NewObject<UNAItemData>(FItemManagerImpl::GetItemSubsystem(OuterHandle), NAME_None, RF_DuplicateTransient);
		}
	}
	return NewItemData;
}

UNAItemGameInstanceSubsystem* FItemManagerImpl::GetItemSubsystem(UWorld* InWorld)
{
	UNAItemGameInstanceSubsystem* ItemGameSubsys = nullptr;
	
	if (InWorld)
	{
		if (UGameInstance* GI = InWorld->GetGameInstance())
		{
			ItemGameSubsys = GI->GetSubsystem<UNAItemGameInstanceSubsystem>();
		}
	}

	return ItemGameSubsys;
}

UNAItemData* FItemManagerImpl::CreateItemDataBySlot( UWorld* InWorld, const FNAInventorySlot& InInventorySlot )
{
	UNAItemData* NewItemData = nullptr;

	if (InInventorySlot.ItemMetaDataKey)
	{
		FDataTableRowHandle ItemMetaDTRowHandle = *(ItemMetaDataMap.Find(InInventorySlot.ItemMetaDataKey.Get()));
		if (ItemMetaDTRowHandle.IsNull()) {
			ensureAlwaysMsgf(false,
				TEXT("[UNAItemGameInstanceSubsystem::CreateItemDataBySlot]  ItemMetaDataMap에 등록되지 않은 ItemActorClass임.  %s"),
				*InInventorySlot.ItemMetaDataKey.Get()->GetName());
			return nullptr;
		}

		NewItemData = NewObject<UNAItemData>(InWorld, NAME_None, RF_Transient);
		if (!NewItemData) {
			ensureAlwaysMsgf(false, TEXT("[UNAItemGameInstanceSubsystem::CreateItemDataBySlot]  새로운 UNAItemData 객체 생성 실패"));
			return nullptr;
		}
		
		NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
		FString NameStr    = ItemMetaDTRowHandle.RowName.ToString();
		FString CountStr   = FString::FromInt(NewItemData->IDCount.GetValue());
		FString NewItemID  = NameStr + TEXT("_") + CountStr;
		
		NewItemData->ID = FName(*NewItemID);
		NewItemData->ItemState = static_cast<EItemState>(InInventorySlot.ItemState);

		// 4) 아이템 데이터 추적용 Map에 새로 생성한 UNAItemData 객체의 소유권 이전
		RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);

		return NewItemData;
	}
	return NewItemData;
}

UNAItemData* FItemManagerImpl::GetRuntimeItemData( const FName& Key ) const
{
	UNAItemData* Value = nullptr;
	Value = RuntimeItemDataMap.Find(Key)->Get();
	return Value;
}

void UNAItemGameInstanceSubsystem::Initialize( FSubsystemCollectionBase& Collection )
{
	Super::Initialize( Collection );
	ManagerImplementation.Initialize();
}
