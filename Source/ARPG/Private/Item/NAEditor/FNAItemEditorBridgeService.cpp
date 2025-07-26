// Fill out your copyright notice in the Description page of Project Settings.
#if WITH_EDITOR

#include "Item/NAEditor/FNAEdItemBridgeService.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "NAEditor_Item/EditorSubsystem/NAEdItemEditorSubsystem.h"
#include "Item/ItemActor/NAItemActor.h"
#include "NAEditor_Misc/NAEdLogCategory.h"
#include "UObject/SavePackage.h"

void FNAEdItemBridgeService::CheckItemSubsystems() const
{
	check(UNAItemEngineSubsystem::Get());
	check(UNAEdItemEditorSubsystem::Get());
}

bool FNAEdItemBridgeService::IsItemActor(const UClass* InClass) const
{
	CheckItemSubsystems();
	if (!IsValid(InClass)) return false;
	return InClass->IsChildOf<ANAItemActor>();
}

bool FNAEdItemBridgeService::IsRegisteredItemMetaClass(const UClass* ItemClass) const
{
	CheckItemSubsystems();
	if (!IsValid(ItemClass)) return false;
	
	if (!UNAItemEngineSubsystem::Get()->IsSoftItemMetaDataInitialized()) return false;
	
	UClass* Key = nullptr;
	if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(ItemClass)))
	{
		Key = BP->GeneratedClass.Get();
	}
	Key = Key ? Key : const_cast<UClass*>(ItemClass);
	
	return ItemClass->IsChildOf<ANAItemActor>() &&
		(UNAItemEngineSubsystem::Get()->ItemMetaData.Contains(Key)
			|| UNAItemEngineSubsystem::Get()->SoftItemMetaData.Contains(Key));
}

bool FNAEdItemBridgeService::IsItemMetaDataInitialized() const
{
	CheckItemSubsystems();
	return UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized();
}

EItemEditorRegistrationPhase FNAEdItemBridgeService::GetItemRegistrationPhase(const UClass* InClass) const
{
	CheckItemSubsystems();
	
	if (!IsValid(InClass)) return EItemEditorRegistrationPhase::None;
	if (!IsRegisteredItemMetaClass(InClass)) return EItemEditorRegistrationPhase::None;
	
	EItemEditorRegistrationPhase Phase = UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
		        ? EItemEditorRegistrationPhase::DuringEditorRuntime
		        : EItemEditorRegistrationPhase::DuringInstancing;
	
	return Phase;
}

void FNAEdItemBridgeService::RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, FName InRowName)
{
	CheckItemSubsystems();
	
	if (InDataTable && InRowName.IsValid())
	{
		// 재검증
		if (IsRegisteredItemMetaClass(NewItemClass))
		{
			UE_LOG(NAEdItem, Warning, TEXT("[%hs] 아이템 메타데이터에 이미 등록된 클래스 : %s")
				, __FUNCTION__, *GetNameSafe(NewItemClass));
			return;
		}
		
		FDataTableRowHandle NewHandle;
		NewHandle.DataTable = InDataTable;
		NewHandle.RowName = InRowName;
		UNAItemEngineSubsystem::Get()->ItemMetaData.Emplace(NewItemClass, NewHandle);
		BroadcastItemClassRegisteredToMetaData(NewItemClass);
	}
}

void FNAEdItemBridgeService::VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, FName InRowName)
{
	CheckItemSubsystems();
	
	if (GetItemRegistrationPhase(ItemClass) == EItemEditorRegistrationPhase::DuringEditorRuntime)
	{
		FDataTableRowHandle& RowHandle = UNAItemEngineSubsystem::Get()->ItemMetaData[ItemClass];
		
		bool bUpdateRowName = false;
		bool bUpdateDataTable = false;
		
		if (RowHandle.IsNull())
		{
			UE_LOG(NAEdItem, Warning, TEXT("[%hs] ItemMetaData 내 '%s' 데이터 유효성 검사 실패. 새 DT 핸들 생성.")
				, __FUNCTION__, *GetNameSafe(ItemClass));
			bUpdateDataTable = true;
		}

		if (RowHandle.DataTable != InDataTable)
		{
			ensureAlwaysMsgf(false,
				TEXT("[%hs] 아이템 클래스-데이터 테이블 불일치. 클래스: %s")
				, __FUNCTION__, *GetNameSafe(ItemClass));
			bUpdateRowName = true;
			bUpdateDataTable = true;
		}
		
		if (RowHandle.RowName != InRowName)
		{
			UE_LOG(NAEdItem, Warning, TEXT("[%hs] ItemMetaDataMap의 RowName 불일치. 업데이트 진행. 클래스: %s, 기존 RowName: %s, 새 RowName: %s")
				, __FUNCTION__, *GetNameSafe(ItemClass), *RowHandle.RowName.ToString(), *InRowName.ToString());
			bUpdateRowName = true;
		}

		if (bUpdateRowName)
		{
			RowHandle.RowName = InRowName;
		}
		if (bUpdateDataTable)
		{
			RowHandle.DataTable = InDataTable;
		}
	}
}

void FNAEdItemBridgeService::MarkMetaDataTableDirty(UClass* ItemClass)
{
	CheckItemSubsystems();
	
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &UNAItemEngineSubsystem::Get()->SoftItemMetaData[ItemClass];
	}
	else
	{
		MetaDataRowHandle = &UNAItemEngineSubsystem::Get()->ItemMetaData[ItemClass];
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

void FNAEdItemBridgeService::SaveMetaDataTable(UClass* ItemClass)
{
	CheckItemSubsystems();
	
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &UNAItemEngineSubsystem::Get()->SoftItemMetaData[ItemClass];
		
	}
	else
	{
		MetaDataRowHandle = &UNAItemEngineSubsystem::Get()->ItemMetaData[ItemClass];
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

TMap<TSoftClassPtr<AActor>, FDataTableRowHandle>& FNAEdItemBridgeService::GetSoftItemMetaData()
{
	CheckItemSubsystems();
	return UNAItemEngineSubsystem::Get()->SoftItemMetaData;
}

TMap<TSubclassOf<AActor>, FDataTableRowHandle>& FNAEdItemBridgeService::GetItemMetaData()
{
	CheckItemSubsystems();
	return UNAItemEngineSubsystem::Get()->ItemMetaData;
}

bool FNAEdItemBridgeService::IsSoftItemMetaDataInitialized() const
{
	CheckItemSubsystems();
	return UNAItemEngineSubsystem::Get()->IsSoftItemMetaDataInitialized();
}

void FNAEdItemBridgeService::SetSoftItemMetaDataInitialized(const bool bInitialized) const
{
	CheckItemSubsystems();
	UNAItemEngineSubsystem::Get()->bSoftItemMetaDataInitialized = bInitialized;
}

void FNAEdItemBridgeService::BroadcastItemClassRegisteredToMetaData(UClass* ItemClass)
{
	CheckItemSubsystems();
	if (!IsValid(ItemClass) || !ItemClass->IsChildOf<ANAItemActor>()) return;
	if (!IsRegisteredItemMetaClass(ItemClass)) return;
	ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(ItemClass->GetDefaultObject(false));
	if (!IsValid(ItemActorCDO)) return;

	ItemActorCDO->HandleItemClassRegisteredToMetaData(GetItemRegistrationPhase(ItemClass));
}

void FNAEdItemBridgeService::SetItemMetaDataInitialized(const bool bInitialized) const
{
	CheckItemSubsystems();
	UNAItemEngineSubsystem::Get()->bItemMetaDataInitialized = bInitialized;
}

/*
#include "NAEditor_Item/EditorSubsystem/NAEdItemEditorSubsystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"

template <typename Predicator, typename PreCompile, typename PostCompile>
void PredicateBlueprintRecompile(const ANAItemActor* InObject, Predicator P = [](ANAItemActor*){ return true; }, PreCompile R = [](UBlueprint*){}, PostCompile C = [](UBlueprint*){})
{
	if ( P( InObject ) )
	{
		if ( UBlueprint* Blueprint = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass( InObject->GetClass() ) ))
		{
			R( Blueprint );
			
			FKismetEditorUtilities::CompileBlueprint(
				Blueprint,
				EBlueprintCompileOptions::SkipSave
				| EBlueprintCompileOptions::SkipGarbageCollection
				| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing
				);

			C( Blueprint );
		}
	}
}

void FNAEdItemUtilities::PredicateBlueprintRecompile_FlagWise(const ANAItemActor* InObject)
{
	const auto& Predicator = []( const ANAItemActor* Obj ) { return Obj->GetDirtySubobjectFlags() != EItemSubobjDirtyFlags::ISDF_None; };
	const auto& PreCompile = []( UBlueprint* BP ){  };
	const auto& PostCompile = []( const UBlueprint* BP )
	{
		BP->MarkPackageDirty();
	};
	
	PredicateBlueprintRecompile( InObject, Predicator, PreCompile, PostCompile );
}

void FNAEdItemUtilities::PredicateBlueprintRecompile_DirtyWise(const ANAItemActor* InObject)
{
	const auto& Predicator = []( const ANAItemActor* Obj )
	{
		if (const UBlueprint* BP = Cast<UBlueprint>( UBlueprint::GetBlueprintFromClass( Obj->GetClass() ) ) )
		{
			return !BP->IsPossiblyDirty();
		}

		return false;
	};
	const auto& PreCompile = []( UBlueprint* BP ){};
	const auto& PostCompile = []( UBlueprint* BP ) { FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); };

	PredicateBlueprintRecompile( InObject, Predicator, PreCompile, PostCompile );
}

const FTableRowBase* FNAEdItemUtilities::FindItemMetaDataForEditingImpl(UClass* ItemClass)
{
	UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (!IsRegisteredItemMetaClass(ItemClass)) return nullptr;

	UClass* BPClassKey = nullptr;
	if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(ItemClass)))
	{
		BPClassKey = BP->GeneratedClass.Get();
	}
	BPClassKey = BPClassKey ? BPClassKey : ItemClass;
	
	return Subsystem->FindItemMetaDataImpl(BPClassKey);
}*/
#endif