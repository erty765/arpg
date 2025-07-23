#include "ItemEditor/NAItemEditorUtilities.h"

#include "ItemEditor/EditorSubsystem/NAItemEditorSubsystem.h"
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

bool FNAItemEditorUtilities::IsRegisteredItemMetaClass(UClass* ItemClass)
{
	const UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (!Subsystem->IsSoftItemMetaDataInitialized()) return false;
	UClass* Key = ItemClass;
	if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(ItemClass)))
	{
		Key = BP->GeneratedClass.Get();
	}
	Key = Key ? Key : ItemClass;
	
	return ItemClass->IsChildOf<ANAItemActor>() &&
		(Subsystem->ItemMetaData.Contains(Key) || Subsystem->SoftItemMetaData.Contains(Key));
}

void FNAItemEditorUtilities::RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, const FName InRowName)
{
	UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (InDataTable && InRowName.IsValid())
	{
		// 재검증
		if (IsRegisteredItemMetaClass(NewItemClass))
		{
			UE_LOG(LogNAItemEditor, Warning, TEXT("[%hs] 아이템 메타데이터에 이미 등록된 클래스 : %s")
				, __FUNCTION__, *GetNameSafe(NewItemClass));
			return;
		}
		
		FDataTableRowHandle NewHandle;
		NewHandle.DataTable = InDataTable;
		NewHandle.RowName = InRowName;
		Subsystem->ItemMetaData.Emplace(NewItemClass, NewHandle);
		if (ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(NewItemClass->GetDefaultObject(false)))
		{
			ItemActorCDO->OnItemClassRegisteredToMetaData.ExecuteIfBound();
		}
	}
}

void FNAItemEditorUtilities::VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, const FName InRowName)
{
	UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (Subsystem->IsItemMetaDataInitialized() && IsRegisteredItemMetaClass(ItemClass))
	{
		decltype(Subsystem->ItemMetaData[ItemClass])& RowHandle = Subsystem->ItemMetaData[ItemClass];
		
		bool bUpdateRowName = false;
		bool bUpdateDataTable = false;
		
		if (RowHandle.IsNull())
		{
			UE_LOG(LogNAItemEditor, Warning, TEXT("[%hs] ItemMetaData 내 '%s' 데이터 유효성 검사 실패. 새 DT 핸들 생성.")
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
			UE_LOG(LogNAItemEditor, Warning, TEXT("[%hs] ItemMetaDataMap의 RowName 불일치. 업데이트 진행. 클래스: %s, 기존 RowName: %s, 새 RowName: %s")
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

void FNAItemEditorUtilities::MarkMetaDataTableDirty(UClass* ItemClass)
{
	const UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!Subsystem->IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &Subsystem->SoftItemMetaData[ItemClass];
		
	}
	else
	{
		MetaDataRowHandle = &Subsystem->ItemMetaData[ItemClass];
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
 
void FNAItemEditorUtilities::SaveMetaDataTable(UClass* ItemClass)
{
	const UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem );
	
	if (!IsRegisteredItemMetaClass(ItemClass)) return;

	const FDataTableRowHandle* MetaDataRowHandle = nullptr;
	if (!Subsystem->IsItemMetaDataInitialized())
	{
		MetaDataRowHandle = &Subsystem->SoftItemMetaData[ItemClass];
		
	}
	else
	{
		MetaDataRowHandle = &Subsystem->ItemMetaData[ItemClass];
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

void FNAItemEditorUtilities::PredicateBlueprintRecompile_FlagWise(const ANAItemActor* InObject)
{
	const auto& Predicator = []( const ANAItemActor* Obj ) { return Obj->GetDirtySubobjectFlags() != EItemSubobjDirtyFlags::ISDF_None; };
	const auto& PreCompile = []( UBlueprint* BP ){  };
	const auto& PostCompile = []( const UBlueprint* BP )
	{
		BP->MarkPackageDirty();
	};
	
	PredicateBlueprintRecompile( InObject, Predicator, PreCompile, PostCompile );
}

void FNAItemEditorUtilities::PredicateBlueprintRecompile_DirtyWise(const ANAItemActor* InObject)
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

const FTableRowBase* FNAItemEditorUtilities::FindItemMetaDataForEditingImpl(UClass* ItemClass)
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
}
