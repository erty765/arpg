#pragma once
#include "Item/ItemDataStructs/NAItemBaseDataStructs.h"

#include "NAItemData.generated.h"

UENUM(BlueprintType)
enum class EItemState : uint8
{
	IS_None            UMETA(DisplayName = "None"),           // 초기화 필요

	IS_Acquired        UMETA(DisplayName = "Acquired"),       // 인벤토리에 획득된 상태
	IS_Active          UMETA(DisplayName = "Active"),         // 사용 가능 상태
	IS_Inactive        UMETA(DisplayName = "Inactive"),       // 사용 불가 (조건 불충족, 쿨다운 등)
	IS_Consumed        UMETA(DisplayName = "Consumed"),       // 사용 후 사라진 상태
	IS_Disabled        UMETA(DisplayName = "Disabled"),       // 의도적으로 비활성화된 상태
	IS_Locked          UMETA(DisplayName = "Locked"),         // 특정 조건을 만족해야 사용할 수 있음
	IS_Expired         UMETA(DisplayName = "Expired"),        // 유효기간 경과 등으로 무효 상태
	IS_Hidden          UMETA(DisplayName = "Hidden"),          // UI 등에서 숨김 처리
	IS_Equipped        UMETA(DisplayName = "Equipped"),          // 착용 중인 장비 아이템 전용
	IS_Broken          UMETA(DisplayName = "Broken"),          // 내구도가 0이 되어 작동하지 않는 상태
	IS_Pending         UMETA(DisplayName = "Pending"),          // 네트워크 처리 중이거나, UI 연출 중 잠시 대기 상태
	IS_PreviewOnly     UMETA(DisplayName = "PreviewOnly"),          // 월드에 있지만 상호작용할 수 없는 상태 (예: 상점 프리뷰)
};

/**
 * @TODO: UNAItemData의 생성 방법 정립하기: UNAItemData의 생성/파괴 생명 주기 관리는 !반드시 UNAItemGameInstanceSubsystem에 의해서만 수행되어야 함!
 * @TODO: 1) ANAItemActor의 PostRegisterAllComponents 단계에서(ANAItemActor::InitItemData_Internal), UNAItemGameInstanceSubsystem::CreateItemData에 의해 생성됨 => 즉 아이템 객체(Data + Actor) 생성 순서가 Actor → Data인 경우 [0]
 * @TODO: 2) 아이템 객체 생성 순서가 Data → Actor인 경우에는? (ex. 플레이어의 인벤토리에 소지된 아이템(: Data로서 존재)... 인벤토리에서 꺼내질 때 Actor가 필요함) [...]
 * @TODO: 3) 월드에서 리젠되는 아이템(채집 아이템 또는 스포너에 의해 동적 스폰되는 몬스터가 소지한 아이템 등)은 생성 당시에 Actor가 필요 없음 → 즉 아이템 Data만 생성되어야 하는 경우 [...]
 */
UCLASS(Transient, BlueprintType)
class ARPG_API UNAItemData final : public UObject
{
	GENERATED_BODY()

	friend class UNAItemEngineSubsystem;
public:
	UNAItemData();

	virtual void PostInitProperties() override;

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	int32 GetQuantity() const { return Quantity; }
	
	UFUNCTION(BlueprintCallable, Category = "Item Data")
	void SetQuantity(const int32 NewQuantity);

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	EItemState GetItemState() const { return ItemState; }
	
	UFUNCTION(BlueprintCallable, Category = "Item Data")
	void SetItemState(EItemState NewItemState);

	template<typename ItemDataStructT = FNAItemBaseTableRow>
		requires TIsDerivedFrom<ItemDataStructT, FNAItemBaseTableRow>::IsDerived
	const ItemDataStructT* GetItemMetaDataStruct() const;

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	FORCEINLINE FName GetItemID() const { return ID; }

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	EItemType GetItemType() const;
	
	bool operator==(const FName& OtherID) const
	{
		return ID == OtherID;
	}

	UClass* GetItemActorClass() const;
	class UNAInventoryComponent* GetOwningInventory() const
	{
		return OwningInventory.Get();
	}
	
	FString GetItemName() const;
	FText GetItemDescription() const;

	class UTexture2D* GetItemIcon() const;
	
	UFUNCTION(BlueprintCallable, Category = "Item Data")
	bool IsPickableItem() const;
	UFUNCTION(BlueprintCallable, Category = "Item Data")
	bool IsStackableItem() const;

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	int32 GetItemMaxSlotStackSize() const;

	UFUNCTION(BlueprintCallable, Category = "Item Data")
	int32 GetMaxInventoryHoldCount() const;

	void SetOwningInventory(UNAInventoryComponent* NewInventory);

	bool TryUseItem(AActor* User);

	bool GetInteractableData(FNAInteractableData& OutData) const;

	bool IsCurrencyItem() const;
	
private:
	UPROPERTY(DuplicateTransient,
		VisibleAnywhere, BlueprintReadOnly, Category = "Item Data", meta=(AllowPrivateAccess = "true"))
	FName ID = NAME_None;  // 아이템 ID (DT 행 이름 + 숫자)

	/** 객체가 생성될 때마다 ++ 하여 ID 를 뽑아 주는 원자적 카운터 */
	static FThreadSafeCounter IDCount;
	int32 IDNumber = -1;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Item Data", meta = (UIMin = 1, UIMax = 100))
	int32 Quantity = 1;	// 이 아이템 인스턴스의 현재 수량

	UPROPERTY(EditAnywhere, Category = "Item Data")
	EItemState ItemState = EItemState::IS_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Meta Data")
	FDataTableRowHandle ItemMetaDataHandle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Ownership")
	TWeakObjectPtr<class UNAInventoryComponent> OwningInventory = nullptr;
};

template <typename ItemDataStructT>
	requires TIsDerivedFrom<ItemDataStructT, FNAItemBaseTableRow>::IsDerived
const ItemDataStructT* UNAItemData::GetItemMetaDataStruct() const
{
	if (ItemMetaDataHandle.IsNull()) return nullptr;
	ItemDataStructT* ItemMetaDataStruct = ItemMetaDataHandle
		.GetRow<ItemDataStructT>(ItemMetaDataHandle.RowName.ToString());
	if (!ItemMetaDataStruct) {
		UE_LOG(LogTemp, Warning, TEXT(
			"[%hs] 아이템 메타데이터 읽기 실패. %s")
			, __FUNCTION__, *ItemMetaDataHandle.ToDebugString());
		return nullptr;
	}
	return ItemMetaDataStruct;
}

