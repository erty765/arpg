// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#if WITH_EDITOR
#include "NAEditor_Item/ItemEditorBridge/NAEdItemBridge.h"

class FNAEdItemBridgeService : public INAEdItemBridge
{
public:
	virtual void CheckItemSubsystems() const override;
	
	virtual bool IsItemActor(const UClass* InClass) const override;
	virtual bool IsRegisteredItemMetaClass(const UClass* ItemClass) const override;
	virtual bool IsItemMetaDataInitialized() const override;

	virtual EItemEditorRegistrationPhase GetItemRegistrationPhase(const UClass* InClass) const override;
	
	virtual void RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, FName InRowName) override;
	virtual void VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, FName InRowName) override;
	virtual void MarkMetaDataTableDirty(UClass* ItemClass) override;
	virtual void SaveMetaDataTable(UClass* ItemClass) override;
	
protected:
	virtual TMap<TSoftClassPtr<AActor>, FDataTableRowHandle>& GetSoftItemMetaData() override;
	virtual TMap<TSubclassOf<AActor>, FDataTableRowHandle>& GetItemMetaData() override;
	
	virtual bool IsSoftItemMetaDataInitialized() const override;
	virtual void SetSoftItemMetaDataInitialized(const bool bInitialized) const override;
	
	virtual void BroadcastItemClassRegisteredToMetaData(UClass* ItemClass) override;
	
	virtual void SetItemMetaDataInitialized(const bool bInitialized) const override;
};

#endif
