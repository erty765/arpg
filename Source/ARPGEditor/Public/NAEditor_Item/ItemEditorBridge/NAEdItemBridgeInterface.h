#pragma once
#include "NAEditor_BridgeTemplates/TNAEdBridgeRegistry.h"

class ARPGEDITOR_API INAEdItemBridge
{
	friend class UNAEdItemEditorSubsystem;
	
public:
	virtual ~INAEdItemBridge() = default;
	virtual void CheckItemSubsystems() const = 0;
	
	virtual bool IsItemActor(const UClass* InClass) const = 0;
	virtual bool IsRegisteredItemMetaClass(UClass* ItemClass) = 0;
	virtual bool IsItemMetaDataInitialized() const = 0;
	
protected:
	virtual TMap<TSoftClassPtr<AActor>, FDataTableRowHandle>& GetSoftItemMetaData() = 0;
	virtual TMap<TSubclassOf<AActor>, FDataTableRowHandle>& GetItemMetaData() = 0;
	
	virtual bool IsSoftItemMetaDataInitialized() const = 0;
	virtual void SetSoftItemMetaDataInitialized(const bool bInitialized) const = 0;
	
	virtual void BroadcastItemClassRegisteredToMetaData(UClass* ItemClass) = 0;
	
	virtual void SetItemMetaDataInitialized(const bool bInitialized) const = 0;
	
public:
	
	//@TODO: 아이템 엔진Subsys과 에디터Subsys 사이의 통신에 필요한 메서드 고민하기

	virtual void RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, FName InRowName) =0;
	virtual void VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, FName InRowName) =0;
	virtual void MarkMetaDataTableDirty(UClass* ItemClass) =0;
	virtual void SaveMetaDataTable(UClass* ItemClass) =0;
	
	virtual void PredicateBlueprintRecompile_FlagWise(const AActor* ItemActor) =0;
	virtual void PredicateBlueprintRecompile_DirtyWise(const AActor* ItemActor) =0;
	
	/*template <typename ItemDTRow_T = FNAItemBaseTableRow>
		requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
	static ItemDTRow_T* FindItemMetaDataForEditing(UClass* ItemClass)
	{
		return const_cast<ItemDTRow_T*>(static_cast<const ItemDTRow_T*>(FindItemMetaDataForEditingImpl(ItemClass)));
	}
	
private:
	static const FTableRowBase* FindItemMetaDataForEditingImpl(UClass* ItemClass);*/
};

using FNAEdItemBridgeRegistry = TNAEdBridgeRegistry<INAEdItemBridge>;