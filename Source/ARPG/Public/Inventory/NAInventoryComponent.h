// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "NAInventoryComponent.generated.h"


UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemAdded				UMETA(DisplayName = "No Item Added"),
	IAR_PartialAmountItemAdded	UMETA(DisplayName = "Partial Amount Item Added"),
	IAR_AllItemAdded			UMETA(DisplayName = "All Item Added"),
};

USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()

	FItemAddResult():
	ActualAmountAdded(0),
	OperationResult(EItemAddResult::IAR_NoItemAdded),
	ResultMessage(FText::GetEmpty()) {}

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32			ActualAmountAdded;

	// 아이템 Add의 결과를 캐시하는 이넘 변수
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult	OperationResult;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText			ResultMessage;

	static FItemAddResult AddedNone(const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult;
		AddedNoneResult.ActualAmountAdded = 0;
		AddedNoneResult.OperationResult = EItemAddResult::IAR_NoItemAdded;
		AddedNoneResult.ResultMessage = ErrorText;

		return AddedNoneResult;
	}
	static FItemAddResult AddedPartial(const int32 PartialAmountAdded, const FText& ErrorText)
	{
		FItemAddResult AddedPartialResult;
		AddedPartialResult.ActualAmountAdded = PartialAmountAdded;
		AddedPartialResult.OperationResult = EItemAddResult::IAR_PartialAmountItemAdded;
		AddedPartialResult.ResultMessage = ErrorText;

		return AddedPartialResult;
	}
	static FItemAddResult AddedAll(const int32 AmountAdded, const FText& Message)
	{
		FItemAddResult AddedAllResult;
		AddedAllResult.ActualAmountAdded = AmountAdded;
		AddedAllResult.OperationResult = EItemAddResult::IAR_AllItemAdded;
		AddedAllResult.ResultMessage = Message;

		return AddedAllResult;
	}
};

class UNAItemData;
/**
 * Handle~: UNAInventoryGameInstanceSubsystem가 제공하는 인벤토리 APT에 대한 래퍼 메서드
 * 위젯 컴포넌트: BeginPlay 때 IniWidget 실행 -> CreateWidget (고정되어있다)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARPG_API UNAInventoryComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNAInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FItemAddResult HandleNonStackableItems(UNAItemData* InItem, int32 RequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	int32 HandleStatckableItems(UNAItemData* InItem, int32 RequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	int32 CalculateWeightAddAmount(UNAItemData* InItem, int32 RequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	int32 CalculateNumberForFullStack(UNAItemData* ExistingItem, int32 InitialRequestedAddAmount);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	UNAItemData* FindMatchingItem(UNAItemData* InItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	UNAItemData* FindNextItemByID(UNAItemData* InItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	UNAItemData* FindNextPartialStack(UNAItemData* InItem) const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	void HandleAddNewItem(UNAItemData* Item, const int32 AmountToAdd);
	
	// @TODO 여기서 제거하는 객체가 Item Data / ItemActor인지 확실히 하기
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	void HandleRemoveSingleItemActor(UNAItemData* InItem);

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	int32 HandleRemoveAmountOfItemActors(UNAItemData* InItem, int32 DesiredAmountToRemove);

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	void SplitExistingStack(UNAItemData* InItem, const int32 AmountToSplit);

	// @TODO: 인벤토리 무게 계산 방법 정립부터 한 뒤, 로직 구체화
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE float GetInventoryTotalWeight() const
	{
		return 0.f;
	}
	
	// @TODO: 인벤토리 무게 계산 방법 정립부터 한 뒤, 로직 구체화
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE float GetInventoryWeightCapacity() const
	{
		return 0.f;
	}
	
	// @TODO: 인벤토리 무게 계산 방법 정립부터 한 뒤, 로직 구체화
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE int32 GetInventorySlotCapacity() const
	{
		return 0;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE TArray<UNAItemData*> GetInventoryContents() const { return InventoryContents; }
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE float GetSlotsCapacity() const { return InventorySlotsCapacity; }
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE float GetWeightCapacity() const { return InventoryWeightCapacity; }

	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE void SetSlotsCapacity(const int32 NewSlotsCapacity) const {}
	UFUNCTION(BlueprintCallable, Category = "Inventory Component")
	FORCEINLINE void SetWeightCapacity(const float NewWeightCapacity) const {}


protected:
	UPROPERTY(VisibleAnywhere, Category = "Inventory Component")
	float InventoryTotalWeight;

	UPROPERTY(VisibleAnywhere, Category = "Inventory Component")
	float InventorySlotsCapacity;

	UPROPERTY(VisibleAnywhere, Category = "Inventory Component")
	float InventoryWeightCapacity;

	UPROPERTY(VisibleAnywhere, Category = "Inventory Component")
	TArray<UNAItemData*> InventoryContents;

public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Inventory Widget  ///////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	void ReleaseInventoryWidget();
	void CollapseInventoryWidget();
	
	
protected:
	UPROPERTY(Transient)
	uint8 bInventoryWidgetVisible : 1 = false;
};
