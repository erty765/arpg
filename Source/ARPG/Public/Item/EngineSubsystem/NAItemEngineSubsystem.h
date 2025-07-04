// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Item/ItemData/NAItemData.h"
#include "EngineUtils.h"
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

UCLASS()
class ARPG_API UNAItemEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UNAItemEngineSubsystem();
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
public:
#if WITH_EDITOR
	bool IsRegisteredItemMetaClass(UClass* ItemClass) const;
	void RegisterNewItemMetaData(UClass* NewItemClass, const UDataTable* InDataTable, const FName InRowName);
	void VerifyItemMetaDataRowHandle(UClass* ItemClass, const UDataTable* InDataTable, const FName InRowName);
#endif
	
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
	const ItemDTRow_T* GetItemMetaDataByClass(UClass* InItemActorClass) const {
		if (!InItemActorClass->IsChildOf<ANAItemActor>()) return nullptr;
		
		if (const FDataTableRowHandle* Value = ItemMetaDataMap.Find(InItemActorClass)) {
			return Value->GetRow<ItemDTRow_T>(Value->RowName.ToString());
		}
		return nullptr;
	}
	
	// template <typename ItemActor_T = ANAItemActor>
	// 	requires TIsDerivedFrom<ItemActor_T, ANAItemActor>::IsDerived
	// const UNAItemData* CreateItemDataByActor(ItemActor_T* InItemActor)
	// {
	// 	if (!InItemActor) return nullptr;
	// 	const bool bIsCDOActor = InItemActor->HasAnyFlags(RF_ClassDefaultObject);
	// 	if (!bIsCDOActor && !IsItemMetaDataInitialized()) return nullptr;
	// 	// 아이템 메타데이터 검색
	// 	UClass* InItemActorClass = InItemActor->GetClass();
	// 	const TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle>::ValueType* ValuePtr
	// 		= ItemMetaDataMap.Find(InItemActorClass);
	// 	if (!ValuePtr) return nullptr;
	// 	FDataTableRowHandle ItemMetaDTRowHandle = *ValuePtr;
	// 	if (ItemMetaDTRowHandle.IsNull()) return nullptr;
	// 	// 아이템 데이터 인스턴스 생성
	// 	UNAItemData* NewItemData
	// 		= NewObject<UNAItemData>(this, NAME_None, RF_Transient);
	// 	if (!NewItemData) return nullptr;
	// 	// 아이템 데이터 ID 초기화
	// 	NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
	// 	FString NameStr = ItemMetaDTRowHandle.RowName.ToString();
	// 	FString CountStr = FString::FromInt(NewItemData->IDCount.GetValue());
	// 	FString NewItemID = NameStr + TEXT("_") + CountStr;
	// 	NewItemData->ID = FName(*NewItemID);
	// 	// 런타임 아이템 데이터 추적을 위해 Map에 등록
	// 	RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);
	// 	return RuntimeItemDataMap[NewItemData->ID].Get();
	// }
	
	template <typename ItemActor_T = ANAItemActor>
		requires TIsDerivedFrom<ItemActor_T, ANAItemActor>::IsDerived
	const UNAItemData* CreateItemDataByActor(ItemActor_T* InItemActor)
	{
		if (!InItemActor) {
			ensureAlwaysMsgf(false, TEXT(
				"[%hs] Invalid ItemActor provided"), __FUNCTION__);
			return nullptr;
		}
		const bool bIsCDOActor = InItemActor->HasAnyFlags(RF_ClassDefaultObject);
		if (!bIsCDOActor && !IsItemMetaDataInitialized()) {
			ensureAlwaysMsgf(false, TEXT(
			"[%hs] Item metadata mapping not initialized"), __FUNCTION__);
			return nullptr;
		}
		// 아이템 메타데이터 검색
		UClass* InItemActorClass = InItemActor->GetClass();
		const TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle>::ValueType* ValuePtr
			= ItemMetaDataMap.Find(InItemActorClass);
		if (!ValuePtr) {
			UE_LOG(LogTemp, Warning, TEXT(
				"[%hs] ItemActorClass not found in ItemMetaDataMap: %s")
				, __FUNCTION__, *InItemActorClass->GetName());
			return nullptr;
		}
		FDataTableRowHandle ItemMetaDTRowHandle = *ValuePtr;
		if (ItemMetaDTRowHandle.IsNull()) {
			ensureAlwaysMsgf(false, TEXT(
					"[%hs] ItemActorClass not registered in ItemMetaDataMap: %s"
				), __FUNCTION__, *InItemActorClass->GetName());
			return nullptr;
		}
		// 아이템 데이터 인스턴스 생성
		UNAItemData* NewItemData
			= NewObject<UNAItemData>(this, NAME_None, RF_Transient);
		if (!NewItemData) {
			ensureAlwaysMsgf(false, TEXT(
					"[%hs] Failed to create UNAItemData object"), __FUNCTION__);
			return nullptr;
		}
		// 아이템 데이터 ID 초기화
		NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
		FString NameStr = ItemMetaDTRowHandle.RowName.ToString();
		FString CountStr = FString::FromInt(NewItemData->IDCount.GetValue());
		FString NewItemID = NameStr + TEXT("_") + CountStr;
		NewItemData->ID = FName(*NewItemID);

		// 런타임 아이템 데이터 맵에 등록
		RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData); {
			UE_LOG(LogTemp, Log, TEXT("[%hs] ItemData created: %s (Actor: %s)")
				 , __FUNCTION__, *NewItemID, *InItemActor->GetName());
		}
		return RuntimeItemDataMap[NewItemData->ID].Get();
	}

	UNAItemData* GetRuntimeItemData(const FName& InItemID) const;
	
	UNAItemData* CreateItemDataCopy(const UNAItemData* SourceItemData);

	// Inventory 관련
	UNAItemData* CreateItemDataBySlot( UWorld* InWorld, const FNAInventorySlot& InInventorySlot );

	/**
	 * 
	 * @param InItemID 
	 * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부
	 * @return 
	 */
	bool DestroyRuntimeItemData(const FName& InItemID, const bool bDestroyItemActor = false);
	/**
	 * 
	 * @param InItemID 
	 * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부.
	 *							  아이템 액터의 생명주기를 명시적으로 조절해야하는 경우 이 플래그를 쓰면 안됨
	 * @return 
	 */
	bool DestroyRuntimeItemData(UNAItemData* InItemData, const bool bDestroyItemActor = false);

	template <typename ItemActorT, typename Func>
		requires TIsDerivedFrom<ItemActorT, ANAItemActor>::IsDerived
	void ForEachItemActorOfClass(UWorld* World, Func&& Predicate)
	{
		if (!World) return;

		for (TActorIterator<ItemActorT> It(World); It; ++It)
		{
			ItemActorT* ItemActor = *It;
			if (IsValid(ItemActor))
			{
				Predicate(ItemActor);
			}
		}
	}
		
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
