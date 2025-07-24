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

UCLASS()
class ARPG_API UNAItemEngineSubsystem : public UEngineSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& Collection) override;
   virtual void Deinitialize() override;

public:
   static UNAItemEngineSubsystem* Get()
   {
      if (GEngine)
      {
         return GEngine->GetEngineSubsystem<UNAItemEngineSubsystem>();
      }
      return nullptr;
   }
    
   FORCEINLINE bool IsItemMetaDataInitialized() const
   {
      return bSoftItemMetaDataInitialized && bItemMetaDataInitialized;
   }

   template<typename ItemDTRow_T = FNAItemBaseTableRow>
      requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
   const ItemDTRow_T* FindItemMetaData(UClass* ItemClass) const
   {
      return const_cast<ItemDTRow_T*>( static_cast<const ItemDTRow_T*>(FindItemMetaDataImpl(ItemClass)) );
   }
   
   const UNAItemData* CreateItemDataByActor(ANAItemActor* ItemActor);

   UNAItemData* GetRuntimeItemData(const FName& InItemID) const;
    
   UNAItemData* CreateItemDataCopy(const UNAItemData* SourceItemData);

   // Inventory 관련
   UNAItemData* CreateItemDataBySlot( UWorld* InWorld, const FNAInventorySlot& InInventorySlot );

   /**
    * @param InItemID 
    * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부
    * @return 
    */
   bool DestroyRuntimeItem(const FName& InItemID, const bool bDestroyItemActor = false, AActor* Instigator = nullptr);
   /**
    * @param InItemID 
    * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부.
    * 아이템 액터의 생명주기를 명시적으로 조절해야하는 경우 이 플래그를 쓰면 안됨
    * @return 
    */
   bool DestroyRuntimeItem(UNAItemData* ItemData, const bool bDestroyItemActor = false, AActor* Instigator = nullptr);

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
   
protected:
   const FTableRowBase* FindItemMetaDataImpl(UClass* ItemClass) const;
   
   FORCEINLINE bool IsSoftItemMetaDataInitialized() const
   {
      return bSoftItemMetaDataInitialized;
   }

   FName CreateItemID(const FString& MetaDataRowName);
   
private:
   // 실제 사용할 DataTable들을 모아둔 DataAsset
   UPROPERTY()
   TObjectPtr<UItemDataTablesAsset> ItemDataTableSourceCollection = nullptr;

   // 소프트 아이템 메타데이터
   UPROPERTY()
   TMap<TSoftClassPtr<AActor>, FDataTableRowHandle> SoftItemMetaData;
   UPROPERTY()
   uint8 bSoftItemMetaDataInitialized : 1 = false;
    
   // 아이템 메타데이터
   UPROPERTY()
   TMap<TSubclassOf<AActor>, FDataTableRowHandle> ItemMetaData;
   UPROPERTY()
   uint8 bItemMetaDataInitialized : 1 = false;

   // 런타임 데이터 매핑
   // 아이템 ID: 런타임 때 아이템 데이터 식별용
   UPROPERTY()
   TMap<FName, TObjectPtr<UNAItemData>> RuntimeItemDataMap;
   
   /** 객체가 생성될 때마다 ++ 하여 ID 를 뽑아 주는 원자적 카운터 */
   FThreadSafeCounter IDCount = 0;

#if WITH_EDITOR
   friend class FNAEdItemBridgeService;
   TSharedPtr<FNAEdItemBridgeService> ItemEditorBridge;
#endif
   
   // 에디터 런타임 중 메타데이터 편집할 때 사용할 인스턴스
   // uint8*: ItemMetaData의 Value가 가리키는 RowStruct
   // FDataTableRowHandle*: ItemMetaData의 Value를 가리키는 포인터
   // GetRowMap()->FindKey(uint8*)으로 Row Name 가져와서 ItemMetaData[Key].RowName 변경하기
   //TMap<uint8*, FDataTableRowHandle*> ItemMetaDataBuffer;
};