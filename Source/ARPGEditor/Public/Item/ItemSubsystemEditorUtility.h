#pragma once

class ANAItemActor;
struct FNAItemBaseTableRow;

struct ARPGEDITOR_API FItemSubsystemEditorUtility
{
	static bool IsRegisteredItemMetaClass(UClass* ItemClass);
	static void RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, FName InRowName);
	static void VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, FName InRowName);
	static void MarkMetaDataTableDirty(UClass* ItemClass);
	static void SaveMetaDataTable(UClass* ItemClass);
	static void PredicateBlueprintRecompile_FlagWise(const ANAItemActor* InObject);
	static void PredicateBlueprintRecompile_DirtyWise(const ANAItemActor* InObject);

	template<typename ItemDTRow_T = FNAItemBaseTableRow>
	   requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
	static ItemDTRow_T* FindItemMetaDataForEditing(UClass* ItemClass)
	{
		return const_cast<ItemDTRow_T*>(static_cast<const ItemDTRow_T*>(FindItemMetaDataForEditingImpl(ItemClass)));
	}

private:
	static const FTableRowBase* FindItemMetaDataForEditingImpl(UClass* ItemClass);
};
