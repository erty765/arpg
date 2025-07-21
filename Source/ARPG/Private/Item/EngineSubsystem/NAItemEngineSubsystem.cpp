// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"

#include "NACharacter.h"
#include "Inventory/DataStructs/NAInventoryDataStructs.h"

#include "Item/ItemActor/NAItemActor.h"
#include "Item/ItemDataStructs/NAWeaponDataStructs.h"

#if WITH_EDITOR
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"
#endif


UNAItemEngineSubsystem::UNAItemEngineSubsystem()
{
}

// 프로그램 시작 시 0 에서 시작
FThreadSafeCounter UNAItemEngineSubsystem::IDCount(0);

void UNAItemEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG( LogInit, Log, TEXT("%hs"), __FUNCTION__ )

	if (ItemDataTableSources.IsEmpty())
	{
		// 1) Registry 에셋 동기 로드
		static const FString RegistryPath = TEXT("/Script/ARPG.ItemDataTablesAsset'/Game/00_ProjectNA/01_Blueprint/00_Actor/MainGame/Items/DA_ItemDataTables.DA_ItemDataTables'");
		UItemDataTablesAsset* Registry = Cast<UItemDataTablesAsset>(StaticLoadObject(UItemDataTablesAsset::StaticClass(), nullptr, *RegistryPath));
	
		if (!Registry)
		{
			UE_LOG(NAItem, Error, TEXT("[%hs] ItemDataTablesAsset 로드 실패. 경로: %s"), __FUNCTION__, *RegistryPath);
			return;
		}
	
		// 2) Registry 안의 SoftObjectPtr<UDataTable> 리스트 순회
		UE_LOG(NAItem, Display, TEXT("[%hs] 아이템 DT 로드 시작"), __FUNCTION__);
		for (const TSoftObjectPtr<UDataTable>& SoftDT : Registry->ItemDataTables)
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
		
		// 3) TSoftClassPtr<T>, FDataTableRowHandle 맵 생성 -> 블루프린트 에셋 로드 전에 메타데이터 미리 인스턴싱
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
#if WITH_EDITOR
				if (ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(
					NewItemActorClass->GetDefaultObject(false)))
				{
					ItemActorCDO->OnItemClassRegisteredToMetaData.ExecuteIfBound();
				}
				// 블루프린트 CDO 동적 초기화 후 재컴파일 -> 동적 초기화한 내용을 블프 에디터 패널에 반영하기 위함
				if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(NewItemActorClass)))
				{
					FKismetEditorUtilities::CompileBlueprint(
						BP,
						EBlueprintCompileOptions::SkipSave
						| EBlueprintCompileOptions::SkipGarbageCollection
						| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing);
				}
#endif
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
#if WITH_EDITOR
		// 에디터가 완전히 켜진 뒤에 한 번만 호출
		FCoreDelegates::OnPostEngineInit.AddUObject(this, &UNAItemEngineSubsystem::HandlePostEngineInit);
#endif
	}
}

void UNAItemEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#if WITH_EDITOR
void UNAItemEngineSubsystem::HandlePostEngineInit()
{
	// 한 번만 실행되도록 바인딩 해제
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// 더티된 모든 패키지를 저장
	FEditorFileUtils::SaveDirtyPackages(
		/*bPromptUserToSave=*/		false,
		/*bSaveMapPackages=*/		true,
		/*bSaveContentPackages=*/	true,
		/*bFastSave=*/				true);
}

bool UNAItemEngineSubsystem::IsRegisteredItemMetaClass(UClass* ItemClass) const
{
	if (!IsSoftItemMetaDataInitialized()) return false;
	UClass* Key = ItemClass;
	if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(ItemClass)))
	{
		Key = BP->GeneratedClass.Get();
	}
	Key = Key ? Key : ItemClass;
	
	return ItemClass->IsChildOf<ANAItemActor>() &&
		(ItemMetaData.Contains(Key) || SoftItemMetaData.Contains(Key));
}

void UNAItemEngineSubsystem::RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, const FName InRowName)
{
	if (InDataTable && InRowName.IsValid())
	{
		// 재검증
		if (IsRegisteredItemMetaClass(NewItemClass))
		{
			UE_LOG(NAItem, Warning, TEXT("[%hs] 아이템 메타데이터에 이미 등록된 클래스 : %s")
				, __FUNCTION__, *GetNameSafe(NewItemClass));
			return;
		}
		FDataTableRowHandle NewHandle;
		NewHandle.DataTable = InDataTable;
		NewHandle.RowName = InRowName;
		ItemMetaData.Emplace(NewItemClass, NewHandle);
		if (ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(NewItemClass->GetDefaultObject(false)))
		{
			ItemActorCDO->OnItemClassRegisteredToMetaData.ExecuteIfBound();
		}
	}
}

void UNAItemEngineSubsystem::VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, const FName InRowName)
{
	if (IsItemMetaDataInitialized() && IsRegisteredItemMetaClass(ItemClass))
	{
		bool bUpdateRowName = false;
		bool bUpdateDataTable = false;
		
		if (ItemMetaData[ItemClass].IsNull())
		{
			UE_LOG(NAItem, Warning, TEXT("[%hs] ItemMetaData 내 '%s' 데이터 유효성 검사 실패. 새 DT 핸들 생성.")
				, __FUNCTION__, *GetNameSafe(ItemClass));
			bUpdateDataTable = true;
		}

		if (ItemMetaData[ItemClass].DataTable != InDataTable)
		{
			ensureAlwaysMsgf(false,
				TEXT("[%hs] 아이템 클래스-데이터 테이블 불일치. 클래스: %s")
				, __FUNCTION__, *GetNameSafe(ItemClass));
			bUpdateRowName = true;
			bUpdateDataTable = true;
		}
		
		if (ItemMetaData[ItemClass].RowName != InRowName)
		{
			UE_LOG(NAItem, Warning, TEXT("[%hs] ItemMetaDataMap의 RowName 불일치. 업데이트 진행. 클래스: %s, 기존 RowName: %s, 새 RowName: %s")
				, __FUNCTION__, *GetNameSafe(ItemClass), *ItemMetaData[ItemClass].RowName.ToString(), *InRowName.ToString());
			bUpdateRowName = true;
		}

		if (bUpdateRowName)
		{
			ItemMetaData[ItemClass].RowName = InRowName;
		}
		if (bUpdateDataTable)
		{
			ItemMetaData[ItemClass].DataTable = InDataTable;
		}
	}
}

void UNAItemEngineSubsystem::MarkMetaDataTableDirty(UClass* ItemClass) const
{
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &SoftItemMetaData[ItemClass];
		
	}
	else
	{
		MetaDataRowHandle = &ItemMetaData[ItemClass];
	}
	
	if (MetaDataRowHandle && !MetaDataRowHandle->IsNull())
	{
		UPackage* ItemMetaDTPackage = MetaDataRowHandle->DataTable->GetPackage();
		if (!ItemMetaDTPackage) return;

		if (!ItemMetaDTPackage->IsDirty())
		{
			ItemMetaDTPackage->MarkPackageDirty();
		}
	}
}
 
void UNAItemEngineSubsystem::SaveMetaDataTable(UClass* ItemClass) const
{
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &SoftItemMetaData[ItemClass];
		
	}
	else
	{
		MetaDataRowHandle = &ItemMetaData[ItemClass];
	}
	
	if (MetaDataRowHandle && !MetaDataRowHandle->IsNull())
	{
		const UDataTable* ItemMetaDT = MetaDataRowHandle->DataTable.Get();
		if (!ItemMetaDT) return;
		
		UPackage* ItemMetaDTPackage = ItemMetaDT->GetPackage();
		if (!ItemMetaDTPackage) return;

		if (!ItemMetaDTPackage->IsDirty())
		{
			ItemMetaDTPackage->MarkPackageDirty();
		}

		FString PackageFilePath = FPackageName::LongPackageNameToFilename(
			ItemMetaDTPackage->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bSlowTask = true;

		ItemMetaDTPackage->Save(
			ItemMetaDTPackage
			, const_cast<UDataTable*>(ItemMetaDT)
			, *PackageFilePath
			, SaveArgs);
	}
}
#endif

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

