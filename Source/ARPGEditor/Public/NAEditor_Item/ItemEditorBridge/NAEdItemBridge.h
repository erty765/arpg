#pragma once
#include "NAEditor_BridgeTemplates/TNAEdBridgeRegistry.h"
#include "NAEditor_Item/ItemEditorCommonTypes.h"

//struct FNAItemBaseTableRow;

class ARPGEDITOR_API INAEdItemBridge
{
    friend struct FNAEdItemBridge;
	friend class UNAEdItemEditorSubsystem;
	
public:
	virtual ~INAEdItemBridge() = default;
	
	virtual void CheckItemSubsystems() const = 0;
	
	virtual bool IsItemActor(const UClass* InClass) const = 0;
	virtual bool IsRegisteredItemMetaClass(const UClass* ItemClass) const = 0;
	virtual bool IsItemMetaDataInitialized() const = 0;

	virtual EItemEditorRegistrationPhase GetItemRegistrationPhase(const UClass* InClass) const = 0;
	
	virtual void RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, FName InRowName) = 0;
	virtual void VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, FName InRowName) = 0;
	virtual void MarkMetaDataTableDirty(UClass* ItemClass) = 0;
	virtual void SaveMetaDataTable(UClass* ItemClass) = 0;
	
	/*virtual void PredicateBlueprintRecompile_FlagWise(const AActor* ItemActor) = 0;
    virtual void PredicateBlueprintRecompile_DirtyWise(const AActor* ItemActor) = 0;
    template <typename ItemDTRow_T = FNAItemBaseTableRow>
    	requires std::is_base_of_v<FNAItemBaseTableRow, ItemDTRow_T>
    ItemDTRow_T* FindItemMetaDataForEditing(UClass* ItemClass)
    {
    	return const_cast<ItemDTRow_T*>(static_cast<const ItemDTRow_T*>(FindItemMetaDataForEditingImpl(ItemClass)));
    }
private:
	virtual FTableRowBase* FindItemMetaDataForEditingImpl(UClass* ItemClass) = 0;*/
    
    virtual FTableRowBase* FindItemMetaDataForEditing(UClass* ItemClass) = 0;
    
protected:
	virtual TMap<TSoftClassPtr<AActor>, FDataTableRowHandle>& GetSoftItemMetaData() = 0;
	virtual TMap<TSubclassOf<AActor>, FDataTableRowHandle>& GetItemMetaData() = 0;
	
	virtual bool IsSoftItemMetaDataInitialized() const = 0;
	virtual void SetSoftItemMetaDataInitialized(const bool bInitialized) const = 0;
	
	virtual void BroadcastItemClassRegisteredToMetaData(UClass* ItemClass) = 0;
	
	virtual void SetItemMetaDataInitialized(const bool bInitialized) const = 0;

private:
    
};

DECLARE_NA_EDITOR_BRIDGE_WRAPPER(FNAEdItemBridge, INAEdItemBridge)
#include "BridgeWrapper/INAEdItemBridge.wrapper.h"