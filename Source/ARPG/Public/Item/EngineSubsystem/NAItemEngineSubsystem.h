// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Item/ItemData/NAItemData.h"
#include "EngineUtils.h"
#include "Misc/NALogCategory.h"
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

   // WITH_EDITOR 전용
   template<typename ItemDTRow_T = FNAItemBaseTableRow>
      requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
   ItemDTRow_T* GetItemMetaDataStructs(UClass* InItemActorClass) const
   {
      if (!InItemActorClass->IsChildOf<ANAItemActor>()) return nullptr;
      if (!IsItemMetaDataInitialized()) return nullptr;
      
      UClass* Key = InItemActorClass;
      if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(InItemActorClass))
      {
         if (UBlueprint* BP = Cast<UBlueprint>(BPClass->ClassGeneratedBy))
         {
            Key = BP->GeneratedClass.Get();
         }
      }
      Key = Key ? Key : InItemActorClass;

      if (const FDataTableRowHandle* Value = ItemMetaDataMap.Find(Key))
      {
         return Value->GetRow<ItemDTRow_T>(Value->RowName.ToString());
      }
      return nullptr;
   }
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
      return bSoftMetaDataInitialized && bMetaDataInitialized;
   }

   template<typename ItemDTRow_T = FNAItemBaseTableRow>
      requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
   const ItemDTRow_T* GetItemMetaDataByClass(UClass* InItemActorClass) const
   {
      if (!InItemActorClass->IsChildOf<ANAItemActor>()) return nullptr;
      if (!IsSoftItemMetaDataInitialized()) return nullptr;
       
      UClass* Key = InItemActorClass;
#if WITH_EDITOR || WITH_EDITORONLY_DATA
      if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(InItemActorClass))
      {
         if (UBlueprint* BP = Cast<UBlueprint>(BPClass->ClassGeneratedBy))
         {
            Key = BP->GeneratedClass.Get();
         }
      }
      Key = Key ? Key : InItemActorClass;
#endif
      if (!IsItemMetaDataInitialized())
      {
         if (const FDataTableRowHandle* Value = SoftItemMetaData.Find(Key))
         {
            return Value->GetRow<ItemDTRow_T>(Value->RowName.ToString());
         }
      }
      else
      {
         if (const FDataTableRowHandle* Value = ItemMetaDataMap.Find(Key))
         {
            return Value->GetRow<ItemDTRow_T>(Value->RowName.ToString());
         }
      }
      return nullptr;
   }

   template<typename ItemActor_T = ANAItemActor>
      requires TIsDerivedFrom< ItemActor_T, ANAItemActor>::IsDerived
   const UNAItemData* CreateItemDataByActor(ItemActor_T* InItemActor)
   {
      if (!InItemActor)
      {
         ensureAlwaysMsgf(false, TEXT("[%hs] 유효하지 않은 ANAItemActor."), __FUNCTION__);
         return nullptr;
      }
       
      const bool bIsCDOActor = InItemActor->HasAnyFlags(RF_ClassDefaultObject);
       
      if (!bIsCDOActor && !IsItemMetaDataInitialized())
      {
         ensureAlwaysMsgf(
            false, TEXT("[%hs] 메타데이터 초기화 안됨."), __FUNCTION__);
         return nullptr;
      }
       
      UClass* InItemActorClass = InItemActor->GetClass();

      // 1) 아이템 메타데이터 검색
      const TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle>::ValueType* ValuePtr = ItemMetaDataMap.Find(InItemActorClass);
      if (!ValuePtr)
      {
         ensureAlwaysMsgf(false,
                TEXT("[%hs] ItemMetaDataMap에 ItemActorClass 미등록."), __FUNCTION__);
         return nullptr;
      }
      FDataTableRowHandle ItemMetaDTRowHandle = *ValuePtr;
      if (ItemMetaDTRowHandle.IsNull())
      {
         ensureAlwaysMsgf(
            false,
            TEXT(
               "[%hs] 메타데이터에 등록되지 않은 ItemClass(%s)."
            ), __FUNCTION__, *GetNameSafe(InItemActorClass));
         return nullptr;
      }

      // 2) UNAItemData 객체 생성 및 초기화
      UNAItemData* NewItemData = NewObject<UNAItemData>(this, NAME_None, RF_Transient);
      if (!NewItemData)
      {
         ensureAlwaysMsgf(
            false, TEXT("[%hs] 새 UNAItemData 객체 생성 실패"), __FUNCTION__);
         return nullptr;
      }

      NewItemData->ItemMetaDataHandle = ItemMetaDTRowHandle;
      FString NameStr;
      NameStr += ItemMetaDTRowHandle.RowName.ToString();
      FString CountStr = FString::FromInt(NewItemData->IDCount.GetValue());
      FString NewItemID = NameStr + TEXT("_") + CountStr;
      NewItemData->ID = FName(*NewItemID); 

      // 3) 새로 생성한 UNAItemData 객체의 소유권을 런타임 때 아이템 데이터 추적용 Map으로 이관
      RuntimeItemDataMap.Emplace(NewItemData->ID, NewItemData);

      {
         UE_LOG(NAItem, Warning, TEXT("[%hs] 아이템 데이터 생성 완료. ID: %s, 관련 액터: %s"),
            __FUNCTION__, *NewItemID, *GetNameSafe(InItemActor));
      }
       
      return RuntimeItemDataMap[NewItemData->ID].Get();
   }

   UNAItemData* GetRuntimeItemData(const FName& InItemID) const;
    
   UNAItemData* CreateItemDataCopy(const UNAItemData* SourceItemData);

   // Inventory 관련
   UNAItemData* CreateItemDataBySlot( UWorld* InWorld, const FNAInventorySlot& InInventorySlot );

   /**
    * * @param InItemID 
    * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부
    * @return 
    */
   bool DestroyRuntimeItemData(const FName& InItemID, const bool bDestroyItemActor = false/*, AActor* Instigator = nullptr*/);
   /**
    * * @param InItemID 
    * @param bDestroyItemActor : 해당 아이템 데이터를 참조하는(ID값으로 검색) 아이템 액터를 찾아서 파괴할지 여부.
    * 아이템 액터의 생명주기를 명시적으로 조절해야하는 경우 이 플래그를 쓰면 안됨
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
   
protected:
   FORCEINLINE bool IsSoftItemMetaDataInitialized() const {
      return bSoftMetaDataInitialized;
   }
       
private:
   // 실제 사용할 DataTable 포인터 보관
   UPROPERTY()
   TArray<TObjectPtr<UDataTable>> ItemDataTableSources;

   UPROPERTY()
   TMap<TSoftClassPtr<ANAItemActor>, FDataTableRowHandle> SoftItemMetaData;

   UPROPERTY()
   uint8 bSoftMetaDataInitialized : 1 = false;
    
   // 메타데이터 매핑
   UPROPERTY()
   TMap<TSubclassOf<ANAItemActor>, FDataTableRowHandle> ItemMetaDataMap;
    
   UPROPERTY()
   uint8 bMetaDataInitialized : 1 = false;

   // 런타임 데이터 매핑
   // 아이템 ID: 런타임 때 아이템 데이터 식별용
   UPROPERTY()
   TMap<FName, TObjectPtr<UNAItemData>> RuntimeItemDataMap;
};