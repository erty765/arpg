// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"

#include "Misc/NALogCategory.h"

#include "NACharacter.h"
#include "Inventory/DataStructs/NAInventoryDataStructs.h"

#include "Item/ItemActor/NAItemActor.h"
#include "Item/ItemDataStructs/NAWeaponDataStructs.h"

#if WITH_EDITOR
#include "Item/Editor/FNAItemEditorBridgeService.h"
#include "ItemEditor/ItemEditorBridge/NAItemEditorBridgeRegistry.h"
#endif

// 프로그램 시작 시 0 에서 시작
FThreadSafeCounter UNAItemEngineSubsystem::IDCount(0);

void UNAItemEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG( LogInit, Log, TEXT("%hs"), __FUNCTION__ )

	if (ItemDataTableSourceCollection == nullptr)
	{
		// 1) Registry 에셋 동기 로드
		static const FString RegistryPath = TEXT("/Script/ARPG.ItemDataTablesAsset'/Game/00_ProjectNA/01_Blueprint/00_Actor/MainGame/Items/DA_ItemDataTables.DA_ItemDataTables'");
		ItemDataTableSourceCollection = Cast<UItemDataTablesAsset>(StaticLoadObject(UItemDataTablesAsset::StaticClass(), nullptr, *RegistryPath));
	
		if (!IsValid(ItemDataTableSourceCollection))
		{
			UE_LOG(NAItem, Error, TEXT("[%hs] ItemDataTablesAsset 로드 실패. 경로: %s"), __FUNCTION__, *RegistryPath);
			return;
		}
	
		// 2) Registry 안의 SoftObjectPtr<UDataTable> 리스트 순회
		if (ItemDataTableSourceCollection->ItemDataTables.Num() == 0)
		{
			UE_LOG(NAItem, Error, TEXT("[%hs] ItemDataTablesAsset에 DataTable 에셋이 없음"), __FUNCTION__);
			return;
		}
		TArray<UDataTable*> ItemDataTableSources;
		ItemDataTableSources.Reserve(ItemDataTableSourceCollection->ItemDataTables.Num());
		UE_LOG(NAItem, Display, TEXT("[%hs] 아이템 DT 로드 시작"), __FUNCTION__);
		for (const TSoftObjectPtr<UDataTable>& SoftDT : ItemDataTableSourceCollection->ItemDataTables)
		{
			UDataTable* ResourceDT = SoftDT.LoadSynchronous();
			if (!ResourceDT)
			{
				UE_LOG(NAItem, Warning,
				TEXT("[%hs] 아이템 DT 로드 실패. 경로: %s"), __FUNCTION__, *SoftDT.ToString());
				continue;
			}
	
			ItemDataTableSources.Emplace(ResourceDT);
			UE_LOG(NAItem, Display,
			 TEXT("[%hs] 아이템 DT 로드 성공. 이름: %s"), __FUNCTION__, *ResourceDT->GetName());
		}

		if (ItemDataTableSources.IsEmpty()) return;
		
		// 3) TSoftClassPtr<T>, FDataTableRowHandle 맵 생성 -> 블루프린트 에셋 로드 전에 소프트 메타데이터 미리 인스턴싱
		UE_LOG(NAItem, Display,
		       TEXT("[%hs] SoftClass 메타데이터 매핑 진행"), __FUNCTION__);
		for (UDataTable* DT : ItemDataTableSources)
		{
			for (const TPair<FName, uint8*>& Pair : DT->GetRowMap())
			{
				FName  RowName = Pair.Key;
				FNAItemBaseTableRow* Row = DT->FindRow<FNAItemBaseTableRow>(RowName, TEXT("Mapping [soft] item meta data"));
				if (!Row || Row->ItemClass.IsNull()) continue; 
				FDataTableRowHandle Handle;
				Handle.DataTable = DT;
				Handle.RowName = RowName;
				SoftItemMetaData.Emplace(Row->ItemClass, Handle);
			}
		}
	
		// 4) 메타데이터 맵 빌드
		UE_LOG(NAItem, Display,
			 TEXT("[%hs] 아이템 메타데이터 매핑 진행"), __FUNCTION__);
		if ( !SoftItemMetaData.IsEmpty() && ItemMetaData.IsEmpty())
		{
			bSoftMetaDataInitialized = true;
			ItemMetaData.Reserve(SoftItemMetaData.Num());
			
			for (const auto& Pair : SoftItemMetaData)
			{
				UClass* NewItemActorClass = Pair.Key.LoadSynchronous();
				if (NewItemActorClass && !Pair.Value.IsNull())
				{
					ItemMetaData.Emplace(NewItemActorClass, Pair.Value);
				}
			}
		}
	}

	if (bSoftMetaDataInitialized
		&& SoftItemMetaData.Num() == ItemMetaData.Num())
	{
		bMetaDataInitialized = true;
		UE_LOG(NAItem, Display, TEXT("[%hs] 아이템 메타데이터 인스턴싱 완료"), __FUNCTION__);
	}
	
#if WITH_EDITOR
	ItemEditorBridgeService = MakeShared<FNAItemEditorBridgeService>();
	FNAItemEditorBridgeRegistry::RegisterBridgeService(ItemEditorBridgeService.Get());
#endif
}

void UNAItemEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
#if WITH_EDITOR
	FNAItemEditorBridgeRegistry::UnregisterBridgeService();
	ItemEditorBridgeService.Reset();
#endif
}

UNAItemData* UNAItemEngineSubsystem::CreateItemDataCopy(const UNAItemData* SourceItemData)
{
    if (!IsValid(SourceItemData))
    {
       ensureAlwaysMsgf(false, TEXT("[%hs] 원본 아이템 데이터 무효."), __FUNCTION__);
       return nullptr;
    }

    if (SourceItemData->ItemMetaDataHandle.IsNull())
    {
       ensureAlwaysMsgf(false, TEXT("[%hs] 원본 아이템 메타데이터 핸들 무효."), __FUNCTION__);
       return nullptr;
    }
    
    // 1) DuplicateObject로 원본을 복제 (생성자 로직은 실행되지 않음).
    UNAItemData* Duplicated = DuplicateObject<UNAItemData>(SourceItemData, this);
    if (!Duplicated)
    {
       ensureAlwaysMsgf(false, TEXT("[%hs] 아이템 데이터 복제 실패."), __FUNCTION__);
       return nullptr;
    }

    // 2) ID를 “RowName + NewNumber” 형태로 다시 세팅
    Duplicated->ID = CreateItemID(Duplicated->ItemMetaDataHandle.RowName.ToString());

    // 3) 새로 생성한 UNAItemData 객체의 소유권을 런타임 때 아이템 데이터 추적용 Map으로 이관
    RuntimeItemDataMap.Emplace(Duplicated->ID, Duplicated);

    UE_LOG(NAItem, Warning, TEXT("[%hs] 아이템 데이터 복제 완료. 새 ID: %s, 원본 ID: %s"), __FUNCTION__
    	, *Duplicated->ID.ToString(), *SourceItemData->ID.ToString());
    
    return RuntimeItemDataMap[Duplicated->ID].Get();
}

const FTableRowBase* UNAItemEngineSubsystem::FindItemMetaDataImpl(UClass* ItemClass) const
{
	if (!ItemClass->IsChildOf<ANAItemActor>()) return nullptr;
	if (!IsSoftItemMetaDataInitialized()) return nullptr;
       
	UClass* Key = ItemClass;
	if (!IsItemMetaDataInitialized())
	{
		if (const FDataTableRowHandle* Value = SoftItemMetaData.Find(Key))
		{
			return Value->GetRow<FTableRowBase>(Value->RowName.ToString());
		}
	}
	else
	{
		if (const FDataTableRowHandle* Value = ItemMetaData.Find(Key))
		{
			return Value->GetRow<FTableRowBase>(Value->RowName.ToString());
		}
	}
	return nullptr;
}

const UNAItemData* UNAItemEngineSubsystem::CreateItemDataByActor(ANAItemActor* ItemActor)
{
	if (!ItemActor)
	{
		ensureAlwaysMsgf(false, TEXT("[%hs] 유효하지 않은 ANAItemActor."), __FUNCTION__);
		return nullptr;
	}
       
	const bool bIsCDOActor = ItemActor->HasAnyFlags(RF_ClassDefaultObject);
	if (!bIsCDOActor && !IsItemMetaDataInitialized())
	{
		ensureAlwaysMsgf(
		   false, TEXT("[%hs] 메타데이터 초기화 안됨."), __FUNCTION__);
		return nullptr;
	}
       
	UClass* ItemClass = ItemActor->GetClass();
	if (!ItemClass->IsChildOf<ANAItemActor>())
	{
		ensureAlwaysMsgf(
		   false, TEXT("[%hs] '%s(class: %s)'는 ANAItemActor 파생 객체가 아님."), __FUNCTION__
		   , *GetNameSafe(ItemActor), *GetNameSafe(ItemClass));
		return nullptr;
	}

	// 1) 아이템 메타데이터 검색
	const TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle>::ValueType* ValuePtr = ItemMetaData.Find(ItemClass);
	if (!ValuePtr)
	{
		ensureAlwaysMsgf(false,
			   TEXT("[%hs] ItemMetaDataMap에 ItemActorClass 미등록."), __FUNCTION__);
		return nullptr;
	}
	FDataTableRowHandle ItemMetaDTRowHandle = *ValuePtr;
	if (ItemMetaDTRowHandle.IsNull())
	{
		ensureAlwaysMsgf(
		   false,
		   TEXT(
			  "[%hs] 메타데이터에 등록되지 않은 ItemClass(%s)."
		   ), __FUNCTION__, *GetNameSafe(ItemClass));
		return nullptr;
	}

	// 2) UNAItemData 객체 생성 및 초기화
	UNAItemData* NewItemData = NewObject<UNAItemData>(this, NAME_None, RF_Transient);
	if (!NewItemData)
	{
		ensureAlwaysMsgf(
		   false, TEXT("[%hs] 새 UNAItemData 객체 생성 실패"), __FUNCTION__);
		return nullptr;
	}
      
	NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
	NewItemData->ID = CreateItemID(ItemMetaDTRowHandle.RowName.ToString());

	// 3) 새로 생성한 UNAItemData 객체의 소유권을 런타임 때 아이템 데이터 추적용 Map으로 이관
	RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);

	{
		UE_LOG(NAItem, Warning, TEXT("[%hs] 아이템 데이터 생성 완료. ID: %s, 관련 액터: %s"),
		   __FUNCTION__, *NewItemData->ID.ToString(), *GetNameSafe(ItemActor));
	}
       
	return RuntimeItemDataMap[NewItemData->ID].Get();
}

UNAItemData* UNAItemEngineSubsystem::GetRuntimeItemData(const FName& InItemID) const
{
    UNAItemData* Value = nullptr;
    if (!InItemID.IsNone())
    {
       Value = RuntimeItemDataMap.Find(InItemID)->Get();
    }
    return Value;
}

UNAItemData* UNAItemEngineSubsystem::CreateItemDataBySlot(UWorld* InWorld, const FNAInventorySlot& InInventorySlot)
{
    if (InInventorySlot.ItemMetaDataKey)
    {
       FDataTableRowHandle ItemMetaDTRowHandle = *(ItemMetaData.Find(InInventorySlot.ItemMetaDataKey.Get()));
       if (ItemMetaDTRowHandle.IsNull()) {
          ensureAlwaysMsgf(false,
             TEXT("[%hs] ItemMetaData 미등록 ItemActorClass. 클래스: %s"),
             __FUNCTION__, *InInventorySlot.ItemMetaDataKey.Get()->GetName());
          return nullptr;
       }
       
       UNAItemData* NewItemData = NewObject<UNAItemData>(this, NAME_None, RF_Transient);
       if (!NewItemData) {
          ensureAlwaysMsgf(false, TEXT("[%hs] 새 UNAItemData 객체 생성 실패."), __FUNCTION__);
          return nullptr;
       }
       
       NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
       NewItemData->ID = CreateItemID(ItemMetaDTRowHandle.RowName.ToString());
       NewItemData->ItemState = static_cast<EItemState>(InInventorySlot.ItemState);

       // 3) 새로 생성한 UNAItemData 객체의 소유권을 런타임 때 아이템 데이터 추적용 Map으로 이관
       RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);

       UE_LOG(NAItem, Warning, TEXT("[%hs] 슬롯 데이터로 아이템 데이터 생성 완료. ID: %s")
       	, __FUNCTION__, *NewItemData->ID.ToString());
       
       return RuntimeItemDataMap[NewItemData->ID];
    }

    ensureAlwaysMsgf(false, TEXT("[%hs] InInventorySlot의 ItemMetaDataKey(%s) 무효.")
    	, __FUNCTION__, *GetNameSafe(InInventorySlot.ItemMetaDataKey.Get()));
    return nullptr;
}

bool UNAItemEngineSubsystem::DestroyRuntimeItem(const FName& InItemID, const bool bDestroyItemActor, AActor* Instigator)
{
    bool bResult = RuntimeItemDataMap.Contains(InItemID);
    if (bResult)
    {
       UNAItemData* ItemData = RuntimeItemDataMap[InItemID];
       if (ensureAlways(IsValid(ItemData)))
       {
          ItemData->RemoveFromRoot();
          ItemData->ConditionalBeginDestroy();

          RuntimeItemDataMap[InItemID] =  nullptr;
          int32 bSucceed = RuntimeItemDataMap.Remove(InItemID);
          bResult = bSucceed == 1;
       }
       if (GetWorld() && bDestroyItemActor && IsValid(Instigator))
       {
          ForEachItemActorOfClass<ANAItemActor>(GetWorld(), [InItemID, Instigator](ANAItemActor* ItemActor)
          {
             if (ItemActor->GetItemData()->GetItemID() == InItemID)
             {
             	if (Instigator->HasAuthority())
             	{
             		ItemActor->Destroy();
             	}
                else
                {
                	if (ANACharacter* Character = Cast<ANACharacter>(Instigator))
                	{
                		Character->Server_DestroyItemActor(ItemActor);
                	}
                }
             }
          });
       }
    }
    return bResult;
}

bool UNAItemEngineSubsystem::DestroyRuntimeItem(UNAItemData* ItemData, const bool bDestroyItemActor, AActor* Instigator)
{
    return DestroyRuntimeItem(ItemData->ID, bDestroyItemActor, Instigator);
}

FName UNAItemEngineSubsystem::CreateItemID(const FString& MetaDataRowName)
{
	IDCount.Increment();
	FString NewNumber = FString::FromInt(IDCount.GetValue());
	FString NewID = MetaDataRowName + TEXT("_") + NewNumber;
	return FName(*NewID);
}

