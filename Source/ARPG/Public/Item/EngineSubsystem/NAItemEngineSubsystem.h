// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Item/ItemData/NAItemData.h"
#include "NAItemEngineSubsystem.generated.h"

UCLASS(BlueprintType)
class UItemDataTablesAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 에디터에서 드래그&드롭으로 DataTable을 넣을 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UDataTable>> ItemDataTables;
};

class ANAItemActor;
struct FNAInventorySlot;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemActorCDOPatched, UClass*, const FNAItemBaseTableRow*, EItemMetaDirtyFlags);

UCLASS()
class ARPG_API UNAItemEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UNAItemEngineSubsystem();
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
#if WITH_EDITOR
public:
	void CheckAndMappingItemClass(UClass* InItemActorClass);
	EItemMetaDirtyFlags FindChangedItemMetaFlags(bool bCheckMetaMap, const FDataTableRowHandle& RowHandle, const UObject* InCDO) const;
	EItemMetaDirtyFlags FindChangedItemMetaFlags(bool bCheckMetaMap, const FNAItemBaseTableRow* RowData, const UObject* InCDO) const;
	
protected:
	bool ContainsItemMetaDataHandle(const FDataTableRowHandle& RowHandle) const;
	bool ContainsItemMetaDataEntry(const FNAItemBaseTableRow* RowData) const;
	bool ContainsItemMetaClass(UClass* InItemActorClass) const;
	
	void ValidateItemRow(const FNAItemBaseTableRow* RowData, const FName RowName) const;
	void SynchronizeItemCDOWithMeta(UClass* InItemActorClass, const FNAItemBaseTableRow* RowData, bool bShouldRecompile) const;
	
	UClass* ResolveToSkeletalItemClass(UClass* InItemActorClass) const;
	UClass* ResolveToGeneratedItemClass(UClass* InItemActorClass) const;
	
	void HandlePostEngineInit() const;
	void HandlePostCDOCompiled(UObject* InCDO, const FObjectPostCDOCompiledContext& CompiledContext);
#endif
		

public:
#if WITH_EDITOR
	FOnItemActorCDOPatched OnItemActorCDOPatched;
	
	FNAItemBaseTableRow* AccessItemMetaData(UClass* InItemActorClass) const;
#endif
	const FNAItemBaseTableRow* GetItemMetaData(UClass* InItemActorClass) const;
public:
	static UNAItemEngineSubsystem* Get()
	{
		if (GEngine)
		{
			return GEngine->GetEngineSubsystem<UNAItemEngineSubsystem>();
		}
		
		return nullptr;
	}
	
	FORCEINLINE bool IsItemMetaDataInitialized() const {
		return bMetaDataInitialized;
	}

	template<typename ItemDTRow_T = FNAItemBaseTableRow>
		requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
	const ItemDTRow_T* GetItemMetaDataByClass(UClass* InItemActorClass) const
	{
		if (!InItemActorClass->IsChildOf<ANAItemActor>())
		{
			return nullptr;
		}
		if (const FDataTableRowHandle* Value = ItemMetaDataMap.Find(InItemActorClass))
		{
			return Value->GetRow<ItemDTRow_T>(Value->RowName.ToString());
		}
		return nullptr;
	}

	template<typename ItemActor_T = ANAItemActor>
		requires TIsDerivedFrom< ItemActor_T, ANAItemActor>::IsDerived
	const UNAItemData* CreateItemDataByActor(ItemActor_T* InItemActor)
	{
		if (!InItemActor)
		{
			ensureAlwaysMsgf(false, TEXT("[UNAItemEngineSubsystem::CreateItemDataByActor]  유효하지 않은 ItemActor를 전달받음."));
			return nullptr;
		}
		
		const bool bIsCDOActor = InItemActor->HasAnyFlags(RF_ClassDefaultObject);
		
		if (!bIsCDOActor && !IsItemMetaDataInitialized())
		{
			ensureAlwaysMsgf(
				false, TEXT("[UNAItemEngineSubsystem::CreateItemDataByActor]  아직도 아이템 메타데이터 매핑이 안되어있다고?? 어째서야"));
			return nullptr;
		}
		
		UClass* InItemActorClass = InItemActor->GetClass();

		// 1) 아이템 메타데이터 검색
		const TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle>::ValueType* ValuePtr = ItemMetaDataMap.Find(InItemActorClass);
		if (!ValuePtr)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("[UNAItemEngineSubsystem::CreateItemDataByActor]  ValuePtr was null."));
			return nullptr;
		}
		FDataTableRowHandle ItemMetaDTRowHandle = *ValuePtr;
		if (ItemMetaDTRowHandle.IsNull())
		{
			ensureAlwaysMsgf(
				false,
				TEXT(
					"[UNAItemEngineSubsystem::CreateItemDataByActor]  ItemMetaDataMap에 등록되지 않은 ItemActorClass임.  %s"
				), *InItemActorClass->GetName());
			return nullptr;
		}

		// 2) UNAItemData 객체 생성 및 초기화
		UNAItemData* NewItemData = NewObject<UNAItemData>(this, NAME_None, RF_Transient);
		if (!NewItemData)
		{
			ensureAlwaysMsgf(
				false, TEXT("[UNAItemGameInstanceSubsystem::CreateItemDataByActor]  새로운 UNAItemData 객체 생성 실패"));
			return nullptr;
		}

		NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
		FString NameStr;
		if (bIsCDOActor)
		{
			NameStr = TEXT("CDO_");
		}
		NameStr += ItemMetaDTRowHandle.RowName.ToString();
		FString CountStr = FString::FromInt(NewItemData->IDCount.GetValue());
		FString NewItemID = NameStr + TEXT("_") + CountStr;

		NewItemData->ID = FName(*NewItemID);

		// 3) 새로 생성한 UNAItemData 객체의 소유권을 런타임 때 아이템 데이터 추적용 Map으로 이관
		RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);

		return RuntimeItemDataMap[NewItemData->ID].Get();
	}
	
	UNAItemData* GetRuntimeItemData(const FName& InItemID) const;

	// Inventory 관련
	UNAItemData* CreateItemDataBySlot( UWorld* InWorld, const FNAInventorySlot& InInventorySlot );
	
private:
	// 실제 사용할 DataTable 포인터 보관
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UDataTable>> ItemDataTableSources;

	// 메타데이터 매핑
	UPROPERTY(VisibleAnywhere)
	TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle> ItemMetaDataMap;
	
	UPROPERTY(Transient)
	uint8 bMetaDataInitialized : 1 = false;

	// 런타임 데이터 매핑
	// 아이템 ID: 런타임 때 아이템 데이터 식별용
	UPROPERTY(VisibleAnywhere)
	TMap<FName, TObjectPtr<UNAItemData>> RuntimeItemDataMap;
};
