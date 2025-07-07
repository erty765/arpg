// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Component/NAInventoryComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Inventory/GameInstance/NAInventoryGameInstanceSubsystem.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "Inventory/Widget/NAInventoryWidget.h"
#include "Item/PickableItem/NAWeapon.h"
#include "Item/PickableItem/NAMedPack.h"

// Sets default values for this component's properties
UNAInventoryComponent::UNAInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetEnableGravity(false);
	CanCharacterStepUpOn = ECB_No;
	SetSimulatePhysics(false);
	bApplyImpulseOnDamage = false;
	bReplicatePhysicsToAutonomousProxy = false;
	BodyInstance.SetInertiaConditioningEnabled(false);
	BodyInstance.bGenerateWakeEvents = false;
	BodyInstance.bUpdateMassWhenScaleChanges = false;
	
	SetWidgetSpace(EWidgetSpace::World);
	SetDrawSize(FVector2D(720, 560));
	SetGeometryMode(EWidgetGeometryMode::Cylinder);
	SetCylinderArcAngle(15.f);
	OpacityFromTexture = 1.f;
	SetBlendMode(EWidgetBlendMode::Masked);
	bIsTwoSided = true;
	
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		InitInventorySlotIDs(this);
	}

	static const FSoftClassPath InventoryWidgetClassPath(TEXT("/Game/00_ProjectNA/01_Blueprint/01_Widget/Inventory/BP_NAInventoryWidget.BP_NAInventoryWidget_C"));
	InventoryWidgetClassRef = InventoryWidgetClassPath;

	static const FSoftObjectPath InventoryWidgetMaterialPath(TEXT("/Game/00_ProjectNA/01_Blueprint/01_Widget/Inventory/Materials/M_InventoryWidgetDefault.M_InventoryWidgetDefault"));
	InventoryWidgetMaterialRef = InventoryWidgetMaterialPath;
}

void UNAInventoryComponent::PostInitProperties()
{
	Super::PostInitProperties();

	if ( HasAnyFlags( RF_ClassDefaultObject ) ) return;
	if ( GetOwner() && GetOwner()->HasAnyFlags( RF_ClassDefaultObject ) )
	{
		return;
	}
	
	if (!InventoryWidgetClassRef.IsNull())
	{
		SetWidgetClass(InventoryWidgetClassRef.LoadSynchronous());
	}
}

// Called when the game starts
void UNAInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// SetMaterial은 어느 시점에서 불러도 렌더러를 요구
	SetMaterial(0, InventoryWidgetMaterialRef.LoadSynchronous());
	SetVisibility(false);
	SetWindowVisibility(EWindowVisibility::SelfHitTestInvisible);
	SetWindowFocusable(false);
}

UNAItemData* UNAInventoryComponent::GetItemDataFromSlot(const FName& SlotID) const
{
	const bool bIsInvenSlot = InvenSlotContents.Contains(SlotID);
	const bool bIsWeaponSlot = WeaponSlotContents.Contains(SlotID);

	if (bIsInvenSlot)
	{
		return  InvenSlotContents[SlotID].Get();
	}
	else if (bIsWeaponSlot)
	{
		return WeaponSlotContents[SlotID].Get();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[GetItemDataFromSlot]  유효하지 않은 슬롯ID."));
	return nullptr;
}

void UNAInventoryComponent::SortInvenSlotItems()
{
    // 1) 채워진 Inven_ 슬롯만 모으기
    TArray<TPair<FName, UNAItemData*>> FilledSlots;
    FilledSlots.Reserve(MaxInventorySlotCount);

    for (auto& Pair : InvenSlotContents)
    {
        //const FName& SlotID = Pair.Key;
        //FString SlotStr = SlotID.ToString();
        //if (SlotStr.StartsWith(TEXT("Inven_")) && Pair.Value.IsValid())
    	if (Pair.Value.IsValid())
        {
            FilledSlots.Emplace(Pair.Key, Pair.Value.Get());
        }
    }

    // 2) 정렬: ItemType ↑, ClassName ↑, Quantity ↓
    FilledSlots.Sort([](const TPair<FName, UNAItemData*>& A, const TPair<FName, UNAItemData*>& B) {
        const UNAItemData* ItemA = A.Value;
        const UNAItemData* ItemB = B.Value;
        
        // 2-1) 타입 비교
        EItemType TypeA = ItemA->GetItemType();
        EItemType TypeB = ItemB->GetItemType();
        if (TypeA != TypeB)
        {
            return static_cast<uint8>(TypeA) < static_cast<uint8>(TypeB);
        }
        
        // 2-2) 클래스 이름 비교
        FString ClassNameA = ItemA->GetItemActorClass()->GetName();
        FString ClassNameB = ItemB->GetItemActorClass()->GetName();
        if (ClassNameA != ClassNameB)
        {
            return ClassNameA < ClassNameB;
        }
        
        // 2-3) 같은 클래스면 수량 내림차순
        return ItemA->GetQuantity() > ItemB->GetQuantity();
    });

    // 3) 모든 Inven_ 슬롯을 비우기
    for (int32 i = 0; i < MaxInventorySlotCount; ++i)
    {
        FString SlotNameStr = FString::Printf(TEXT("Inven_%02d"), i);
        FName SlotID = FName(*SlotNameStr);
        InvenSlotContents[SlotID] = nullptr;
    }

    // 4) 정렬된 순서대로 순차적 재배치
    int32 TargetIndex = 0;
    for (auto& Pair : FilledSlots)
    {
        UNAItemData* ItemData = Pair.Value;
        FString NewSlotName = FString::Printf(TEXT("Inven_%02d"), TargetIndex);
        FName NewSlotID = FName(*NewSlotName);

        InvenSlotContents[NewSlotID] = ItemData;
        
        ++TargetIndex;
    }
	
	// 5) UI 쪽에 갱신 알리기
	// if (IsValid(GetInventoryWidget()))
	// {
	// 	for (const auto& Pair : InventoryContents)
	// 	{
	// 		if (SlotButtons.Find(Pair.Key) != nullptr)
	// 		{
	// 			FName SlotID = Pair.Key;
	// 			UNAItemData* ItemData = Pair.Value.Get();
	// 			UButton* SlotButton = SlotButtons[SlotID].Get();
	// 			GetInventoryWidget()->RefreshSlotButtons(ItemData, SlotButton);
	// 		}
	// 	}
	// }
	
	if (IsValid(GetInventoryWidget()))
	{
		GetInventoryWidget()->RefreshSlotWidgets(InvenSlotContents, WeaponSlotContents/*, SlotButtons*/);
		UpdateWidget();
	}
}

// void UNAInventoryComponent::SplitExistingStack(UNAItemData* ItemToSplit, const int32 AmountToSplit)
// {
// 	if (InventoryContents.Num() + 1 <= InventorySlotsCapacity)
// 	{
// 		HandleRemoveAmountOfItems(ItemToSplit, AmountToSplit);
// 		HandleAddNewItem(ItemToSplit, AmountToSplit);
// 	}
// }

TArray<UNAItemData*> UNAInventoryComponent::GetInventoryContents() const
{
	TArray<UNAItemData*> InventoryContentsArray;
	for (const auto& Pair : InvenSlotContents)
	{
		if (Pair.Value.IsValid())
		{
			InventoryContentsArray.Add(Pair.Value.Get());
		}
	}
	return InventoryContentsArray;
}

FNAItemAddResult UNAInventoryComponent::AddNonStackableItem(UNAItemData* InputItem, const TArray<FName>& InEmptySlots)
{
	FString FailReason;
	// 1) 사전 검증: nullptr, MaxSlotStackSize==1인지, Quantity==1인지, MaxInventoryHoldCount 검사 등
	if (!IsValidForNonStackable(InputItem, FailReason))
	{
		return FNAItemAddResult::AddedNone(FText::FromString(FailReason));
	}

	// 2) 빈 슬롯이 하나라도 있어야 추가 가능
	if (InEmptySlots.IsEmpty())
	{
		return FNAItemAddResult::AddedNone(
			FText::FromString(TEXT("인벤토리에 빈 슬롯이 없습니다."))
		);
	}

	// 3) 첫 번째 빈 슬롯에만 추가
	const FName TargetSlot = InEmptySlots[0];
	const bool bSucceed = HandleAddNewItem(InputItem, TargetSlot);
	if (bSucceed)
	{
		// 비-스택형이므로 Always Distributed = 1
		const FText ItemName = FText::FromString(InputItem->GetItemName());
		return FNAItemAddResult::AddedAll(
			1,
			FText::Format(
				FText::FromString(TEXT("'{0}' 아이템을 성공적으로 추가했습니다.")),
				ItemName
			)
		);
	}

	// 4) 만약 HandleAddNewItem에서 실패한 경우
	return FNAItemAddResult::AddedNone(
		FText::FromString(TEXT("알 수 없는 이유로 추가에 실패했습니다."))
	);
}

FNAItemAddResult UNAInventoryComponent::AddStackableItem(UNAItemData* InputItem, const TArray<FName>& PartialSlots, const TArray<FName>& EmptySlots)
{
	// 1) 기본 입력 검증
    if (!InputItem)
    {
        return FNAItemAddResult::AddedNone(
            FText::FromString(TEXT("아이템이 nullptr입니다."))
        );
    }
    const int32 OriginalQuantity = InputItem->GetQuantity();
    if (OriginalQuantity <= 0)
    {
        return FNAItemAddResult::AddedNone(
            FText::FromString(TEXT("추가할 수량이 0 이하입니다."))
        );
    }
    const int32 MaxHold = InputItem->GetMaxInventoryHoldCount();
    if (MaxHold < 0)
    {
        return FNAItemAddResult::AddedNone(
            FText::FromString(TEXT("유효하지 않은 MaxInventoryHoldCount입니다."))
        );
    }

    // 2) Partial 슬롯과 Empty 슬롯 기반으로 실제로 추가 가능한 총량 계산
    int32 AllowedAmount = ComputeDistributableAmount(InputItem, PartialSlots, EmptySlots);
    if (AllowedAmount <= 0)
    {
        return FNAItemAddResult::AddedNone(
            FText::FromString(TEXT("인벤토리 공간 또는 소지 한도가 부족합니다."))
        );
    }

    // 3) 실제 슬롯에 수량 분배 (Partial → Empty)
    // DistributeToSlots: 원본 데이터를 그대로 슬롯에 등록한 경우 - 원본 Quantity 수정 x
	//					  원본에서 복제한 데이터를 슬롯에 등록한 경우 - 원본 Quantity에서 슬롯의 최대 용량만큼 차감
	//					  복제 데이터를 만들어서 슬롯에 아이템 수량을 분배한 뒤,
	//					  -> 빈 슬롯이 남았다: 원본 데이터를 슬롯에 등록 - 원본 Quantity는 해당 슬롯의 용량을 나타냄
	//					  -> 빈 슬롯이 없다: 아이템이 부분 추가되었으므로, 원본 데이터의 Quantity는 차감된 상태로 월드에 잔여
    int32 Distributed = DistributeToSlots(InputItem, AllowedAmount, PartialSlots, EmptySlots);
    if (Distributed <= 0)
    {
        return FNAItemAddResult::AddedNone(
            FText::FromString(TEXT("아이템 분배 중 오류가 발생했습니다."))
        );
    }

    // 4) 결과 메시지: 전부 추가 vs 부분 추가
    const FText ItemName = FText::FromString(InputItem->GetItemName());
	if (Distributed == OriginalQuantity)
	{
		// 원본이 완전히 인벤토리로 옮겨진 케이스 (Quantity는 0)
		return FNAItemAddResult::AddedAll(
			Distributed,
			FText::Format(
				FText::FromString(TEXT("'{0}' 아이템 {1}개를 전부 인벤토리에 추가했습니다.")),
				ItemName,
				Distributed
			)
		);
	}
	else
	{
		// 일부만 옮겨진 케이스 (Quantity는 “남은” 양)
		const int32 Remaining = OriginalQuantity - Distributed;
		return FNAItemAddResult::AddedPartial(
			Distributed,
			FText::Format(
				FText::FromString(TEXT("'{0}' 아이템 중 {1}개만 인벤토리에 추가했습니다. 남은 {2}개")),
				ItemName,
				Distributed,
				Remaining
			)
		);
	}
}

FNAItemAddResult UNAInventoryComponent::AddCurrencyItem(UNAItemData* InputItem)
{
	if (InputItem->GetItemType() == EItemType::IT_PowerNode)
	{
		NodesCount += InputItem->GetQuantity();
		return FNAItemAddResult::AddedAll(
			InputItem->GetQuantity(),
			FText::Format(
				FText::FromString(TEXT("파워 노드 {0}개를 전부 인벤토리에 추가했습니다. 현재 보유한 파워 노드 수량: {1}")),
				InputItem->GetQuantity(),
				NodesCount
			)
		);
	}
	if (InputItem->GetItemType() == EItemType::IT_Credit)
	{
		CreditsCount += InputItem->GetQuantity();
		return FNAItemAddResult::AddedAll(
			InputItem->GetQuantity(),
			FText::Format(
				FText::FromString(TEXT("크레딧 {0}개를 전부 인벤토리에 추가했습니다. 현재 보유한 크레딧 수량: {1}")),
				InputItem->GetQuantity(),
				CreditsCount
			)
		);
	}

	return FNAItemAddResult::AddedNone(
		   FText::FromString(TEXT("Currency 아이템 추가 실패"))
	   );
}

int32 UNAInventoryComponent::GetNumToFillSlot(const FName& SlotID) const
{
	if (!InvenSlotContents.Contains(SlotID) || IsEmptySlot(SlotID))
	{
		return -1;
	}

	const int32 CurrentQuantity = InvenSlotContents[SlotID]->GetQuantity();
	const int32 MaxHoldCount = InvenSlotContents[SlotID]->GetMaxInventoryHoldCount();
	return FMath::Max(MaxHoldCount - CurrentQuantity, 0);
}

// Called every frame
void UNAInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// @TODO: (임시) 인벤토리 슬롯 한 칸에 담긴 아이템 전부 삭제
void UNAInventoryComponent::RemoveItemAtInventorySlot()
{
	if (!GetInventoryWidget()) return;

	FName SlotID = GetInventoryWidget()->GetCurrentFocusedSlotID();
	if (SlotID.IsNone()) return;

	UNAItemData* ItemData = nullptr;
	if (InvenSlotContents.Contains(SlotID))
	{
		ItemData = InvenSlotContents[SlotID].Get();
	}
	else if (WeaponSlotContents.Contains(SlotID))
	{
		ItemData = WeaponSlotContents[SlotID].Get();
	}
	if (!ItemData) return;

	TryRemoveItem(SlotID, ItemData->GetQuantity());
}

void UNAInventoryComponent::UseMedPackAutomatically(AActor* User)
{
	if (!User) return;

	TArray<FName> MedPackSlots;
	GetSlotIDsWithItemClass(ANAMedPack::StaticClass(),MedPackSlots);
	if (MedPackSlots.Num() == 0) return;

	MedPackSlots.Sort([this](const FName& A, const FName& B)
	{
		const UNAItemData* ItemA = InvenSlotContents[A].Get();
		const UNAItemData* ItemB = InvenSlotContents[B].Get();

		const FNARecoveryPackTableRow* RecoveryDataA = ItemA->GetItemMetaDataStruct<FNARecoveryPackTableRow>();
		const FNARecoveryPackTableRow* RecoveryDataB = ItemB->GetItemMetaDataStruct<FNARecoveryPackTableRow>();

		EMedPackGrade GradeA = RecoveryDataA->MedPackGrade;
		EMedPackGrade GradeB = RecoveryDataB->MedPackGrade;
		if (GradeA != GradeB)
		{
			return static_cast<uint8>(GradeA) > static_cast<uint8>(GradeB);
		}

		// @TODO: 같은 등급 슬롯들을 슬롯 넘버의 오름차순으로 따라 정렬
		return static_cast<uint8>(GradeA) == static_cast<uint8>(GradeB);
	});

	UNAItemData* MedPack = InvenSlotContents[MedPackSlots[0]].Get();
	if (!MedPack) return;
	MedPack->TryUseItem(User);
}

void UNAInventoryComponent::UseStasisPackAutomatically(AActor* User)
{
	// @TODO
}

UNAItemData* UNAInventoryComponent::SelectNextWeapon(int32 Direction) const
{
	if (FMath::Abs(Direction) != 1) return nullptr;

	int32 MaxWeaponIndex = MaxWeaponSlotCount - 1;
	int32 NewIndex;
	if (EquippedWeaponIndex == -1)
	{
		NewIndex = Direction > 0 ? 0 : MaxWeaponIndex;
	}
	else
	{
		NewIndex = (EquippedWeaponIndex + Direction + MaxWeaponIndex) % MaxWeaponIndex;
	}
	if (FMath::IsWithinInclusive(NewIndex, 0, MaxWeaponIndex))
	{
		FName WeaponSlotID = MakeWeaponSlotID(NewIndex);
		if (WeaponSlotID.IsValid())
		{
			return GetItemDataFromSlot(WeaponSlotID);
		}
	}
	return nullptr;
}

void UNAInventoryComponent::SetEquippedWeaponIndex(const UNAItemData* EquippedWeapon)
{
	if (EquippedWeapon == nullptr)
	{
		EquippedWeaponIndex = -1;
		return;
	}
	
	FName SlotID = FindSlotIDForItem(EquippedWeapon);
	if (!SlotID.IsNone())
	{
		int32 NewIndex = ExtractSlotNumber(SlotID);
		if (FMath::IsWithinInclusive(NewIndex, 0, MaxWeaponSlotCount))
		{
			EquippedWeaponIndex = NewIndex;
		}
	}
}

int32 UNAInventoryComponent::TryAddItem(UNAItemData* ItemToAdd)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TryAddItem] ItemToAdd가 nullptr입니다."));
		return -1;
	}
	if (!ItemToAdd->IsPickableItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TryAddItem] 인벤토리에 소지 불가능한 아이템입니다."));
		return -1;
	}

	const bool bIsStackable = ItemToAdd->IsStackableItem();
	UClass* ItemClass = ItemToAdd->GetItemActorClass();
	EItemType ItemType = ItemToAdd->GetItemType();

	// 원본 요청 수량을 미리 저장
	const int32 OriginalQuantity = ItemToAdd->GetQuantity();
	
	FNAItemAddResult Result;

	if (ItemToAdd->IsCurrencyItem())
	{
		Result = AddCurrencyItem(ItemToAdd);
	}
	else if (bIsStackable)
	{
		// ─────────────────────────────────────────────────
		// 1) Stackable인 경우: Weapon/Inven 구분 한 뒤, 각 항목의 Partial 슬롯 + Empty 슬롯 수집
		TArray<FName> PartialSlots;
		TArray<FName> EmptySlots;

		if (ItemToAdd->GetItemType() == EItemType::IT_Weapon)
		{
			GatherPartialWeaponSlots(ItemClass, PartialSlots);
			GatherEmptyWeaponSlotsWithClass(ItemClass, EmptySlots);
		}
		else
		{
			GatherPartialInvenSlots(ItemClass, PartialSlots);
			GatherEmptyInvenSlotsWithClass(ItemClass, EmptySlots);
		}
		
		// 2) Internal 함수 호출
		Result = AddStackableItem(ItemToAdd, PartialSlots, EmptySlots);
	}
	else
	{
		// ─────────────────────────────────────────────────
		// 2) Non-stackable인 경우: Weapon/Inven 구분 한 뒤, 각 항목의 Empty 슬롯만 수집
		TArray<FName> EmptySlots;
		if (ItemToAdd->GetItemType() == EItemType::IT_Weapon)
		{
			GatherEmptyWeaponSlotsWithClass(ItemClass, EmptySlots);
		}
		else
		{
			GatherEmptyInvenSlotsWithClass(ItemClass, EmptySlots);
		}

		// 3) Internal 함수 호출
		Result = AddNonStackableItem(ItemToAdd, EmptySlots);
	}
	
	// 반환값을 OriginalQuantity – ActualAmountAdded 형태로 계산
	if (Result.OperationResult == ENAItemAddStatus::IAS_AddedAll ||
		Result.OperationResult == ENAItemAddStatus::IAS_AddedPartial)
	{
		if (ItemToAdd->IsCurrencyItem())
		{
			if (ItemToAdd->GetItemType() == EItemType::IT_PowerNode)
			{
				RequestRedrawNodesQuantity();
			}
			if (ItemToAdd->GetItemType() == EItemType::IT_Credit)
			{
				RequestRedrawCreditsQuantity();
			}
			if (Result.OperationResult == ENAItemAddStatus::IAS_AddedAll)
			{
				UNAItemEngineSubsystem::Get()->DestroyRuntimeItemData(ItemToAdd);
			}
		}
		else
		{
			// 성공(전부 혹은 일부 추가)이므로 정렬 실행
			SortInvenSlotItems();
		}
		
		const int32 Added = Result.ActualAmountAdded;
		// “전부 추가”인 경우 Added == OriginalQuantity 이므로 (Remaining = 0) 
		// “부분 추가”인 경우 Remaining > 0
		return OriginalQuantity - Added;
	}
	else
	{
		// 전혀 추가되지 못했을 때
		return -1;
	}
}

bool UNAInventoryComponent::TryRemoveItem(const FName& SlotID, int32 RequestedAmount)
{
	if (IsEmptySlot(SlotID)) { return false; }
	if (RequestedAmount <= 0) { return false; }
	if (!GetInventoryWidget()) { return false; }
	
	UNAItemData* ItemToRemove = GetItemDataFromSlot(SlotID);
	if (!ItemToRemove) { return false; }
	if (ItemToRemove->GetOwningInventory() != this) { ensure(false); return false; }
	if (ItemToRemove->GetItemMaxSlotStackSize() < RequestedAmount) { return false; }
	
	int32 NewQuantity = ItemToRemove->GetQuantity() - RequestedAmount;
	// 제거 요청된 수량이 현재 수량보다 많은 경우 -> 인벤토리에서 이 아이템 데이터를 빼기
	if (NewQuantity <= 0)
	{
		const bool bSucceed = HandleRemoveItem(SlotID);
		if (!bSucceed) { return false; }
		ItemToRemove = nullptr;
		// if (bSucceed)
		// {
		// 	SortInvenSlotItems();
		// }
		// return bSucceed;
	}
	else
	{
		ItemToRemove->SetQuantity(NewQuantity);
	}
	
	GetInventoryWidget()->RefreshSingleSlotWidget(SlotID, ItemToRemove);
	return true;
}

bool UNAInventoryComponent::HandleRemoveItem(const FName& SlotID)
{
	if (IsEmptySlot(SlotID))
	{
		return false;
	}
	UNAItemData* ItemToRemove = nullptr;
	if (InvenSlotContents.Contains(SlotID))
	{
		ItemToRemove = InvenSlotContents[SlotID].Get();
		if (!ItemToRemove
			|| ItemToRemove->GetOwningInventory() != this) {  return ensureAlways(false); }
		if (ItemToRemove)
		{
			InvenSlotContents[SlotID] = nullptr;
		}
	}
	else if (WeaponSlotContents.Contains(SlotID))
	{
		ItemToRemove = WeaponSlotContents[SlotID].Get();
		if (!ItemToRemove
			|| ItemToRemove->GetOwningInventory() != this) { return ensureAlways(false); }
		
		if (ItemToRemove)
		{
			WeaponSlotContents[SlotID] = nullptr;
		}
	}
	else { return ensureAlways(false); }
	
	if (UNAInventoryGameInstanceSubsystem* InvenSubsys = UNAInventoryGameInstanceSubsystem::Get(GetWorld()))
	{
		UNAItemData* RemovedItem = InvenSubsys->RemoveItemFromInventory(ItemToRemove);
		if (!RemovedItem || ItemToRemove != RemovedItem) { return ensure(false); }
		RemovedItem->SetOwningInventory(nullptr);
		
		// 인벤토리에서 수량이 0으로 변경된 아이템 데이터를 제거
		if (RemovedItem->GetQuantity() <= 0)
		{
			UNAItemEngineSubsystem::Get()->DestroyRuntimeItemData(RemovedItem);
		}
		return true;
	}

	return ensureAlways(false);
}

bool UNAInventoryComponent::HasItemOfClass(const UClass* ItemClass) const
{
	if (!ItemClass) { return false; }
	if (ItemClass->IsChildOf<ANAWeapon>())
	{
		for (const auto& Pair : WeaponSlotContents)
		{
			if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass() == ItemClass)
			{
				return true;
			}
		}
	}
	else
	{
		for (const auto& Pair : InvenSlotContents)
		{
			if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass() == ItemClass)
			{
				return true;
			}
		}
	}
	
	return false;
}

void UNAInventoryComponent::GetSlotIDsWithItemClass(const UClass* ItemClass, TArray<FName>& OutSlotIDs) const
{
	OutSlotIDs.Reset();
	if (!ItemClass) { return; }
	if (ItemClass->IsChildOf<ANAWeapon>())
	{
		for (const auto& Pair : WeaponSlotContents)
		{
			if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass()->IsChildOf(ItemClass))
			{
				OutSlotIDs.Add(Pair.Key);
			}
		}	
	}
	else
	{
		for (const auto& Pair : InvenSlotContents)
		{
			if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass()->IsChildOf(ItemClass))
			{
				OutSlotIDs.Add(Pair.Key);
			}
		}	
	}
}

int32 UNAInventoryComponent::GetSlotRemainingStack(const FName& SlotID) const
{
	// 슬롯에 데이터가 없거나, nullptr이거나, IsEmptySlot일 때
	if (!IsValidSlotID(SlotID) || IsEmptySlot(SlotID))
	{
		return -1;
	}

	int32 CurrentStackSize, MaxStackSize;
	if (InvenSlotContents.Contains(SlotID))
	{
		CurrentStackSize = InvenSlotContents[SlotID]->GetQuantity();
		MaxStackSize = InvenSlotContents[SlotID]->GetItemMaxSlotStackSize();
	}
	else if (WeaponSlotContents.Contains(SlotID))
	{
		CurrentStackSize = WeaponSlotContents[SlotID]->GetQuantity();
		MaxStackSize = WeaponSlotContents[SlotID]->GetItemMaxSlotStackSize();
	}
	else
	{
		check(false);
		return -1;
	}
	
	return FMath::Max(MaxStackSize - CurrentStackSize, 0); // 0 미만 반환 방지
}

bool UNAInventoryComponent::IsEmptySlot(const FName& SlotID) const
{
	if (!SlotID.IsNone())
	{
		if (InvenSlotContents.Contains(SlotID))
		{
			return !InvenSlotContents[SlotID].IsValid();
		}
		else if (WeaponSlotContents.Contains(SlotID))
		{
			return !WeaponSlotContents[SlotID].IsValid();
		}
		else { check(false); }
	}
	return false;
}

void UNAInventoryComponent::GatherEmptyInvenSlots(TArray<FName>& OutEmptySlots) const
{
	OutEmptySlots.Empty();
	OutEmptySlots.Reserve(MaxInventorySlotCount);

	for (const auto& Pair : InvenSlotContents)
	{
		// 값이 비어있으면서(FName→값이 nullptr) “Inven_”으로 시작하는 Key만 수집
		//const FName& SlotID = Pair.Key;
		if (!Pair.Value.IsValid())
		{
			//const FString SlotStr = SlotID.ToString();
			//if (SlotStr.StartsWith(TEXT("Inven_")))
			//{
			OutEmptySlots.Add(Pair.Key);
			//}
		}
	}
}
void UNAInventoryComponent::GatherEmptyWeaponSlots(TArray<FName>& OutEmptySlots) const
{
	OutEmptySlots.Empty();
	OutEmptySlots.Reserve(MaxWeaponSlotCount);

	for (const auto& Pair : WeaponSlotContents)
	{
		// 값이 비어있으면서 “Weapon_”으로 시작하는 Key만 수집
		//const FName& SlotID = Pair.Key;
		if (!Pair.Value.IsValid())
		{
			//const FString SlotStr = SlotID.ToString();
			//if (SlotStr.StartsWith(TEXT("Weapon_")))
			//{
			OutEmptySlots.Add(Pair.Key);
			//}
		}
	}
}

bool UNAInventoryComponent::IsFullSlot(const FName& SlotID) const
{
	if (!IsEmptySlot(SlotID))
	{
		return GetNumToFillSlot(SlotID) <= 0;
	}
	return false;
}

bool UNAInventoryComponent::HasNoEmptySlot() const
{
	for (const auto& Pair : InvenSlotContents)
	{
		if (IsEmptySlot(Pair.Key))
		{
			return false;
		}
	}
	for (const auto& Pair : WeaponSlotContents)
	{
		if (IsEmptySlot(Pair.Key))
		{
			return false;
		}
	}
	return true;
}

bool UNAInventoryComponent::IsAtFullCapacity() const
{
	for (const auto& Pair : InvenSlotContents)
	{
		if (IsEmptySlot(Pair.Key) || !IsFullSlot(Pair.Key))
		{
			return false;
		}
	}
	for (const auto& Pair : WeaponSlotContents)
	{
		if (IsEmptySlot(Pair.Key) || !IsFullSlot(Pair.Key))
		{
			return false;
		}
	}
	return true;
}

void UNAInventoryComponent::GatherPartialInvenSlots(UClass* ItemClass, TArray<FName>& OutPartialSlots) const
{
	if (!ItemClass) { ensure(false); return; }
	if (ItemClass->IsChildOf<ANAWeapon>()) { return; }
	OutPartialSlots.Empty();

	for (const auto& Pair : InvenSlotContents)
	{
		if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass() == ItemClass)
		{
			// 해당 슬롯의 현재 수량이 최대 슬롯 스택에 도달하지 않았으면 후보로 추가
			const int32 CurrQty = Pair.Value->GetQuantity();
			const int32 MaxSlotStack = Pair.Value->GetItemMaxSlotStackSize();
			if (CurrQty < MaxSlotStack)
			{
				OutPartialSlots.Add(Pair.Key);
			}
		}
	}
}

void UNAInventoryComponent::GatherPartialWeaponSlots(UClass* ItemClass, TArray<FName>& OutPartialSlots) const
{
	if (!ItemClass) { ensure(false); return; }
	if (!ItemClass->IsChildOf<ANAWeapon>()) { return; }
	
	OutPartialSlots.Empty();
	for (const auto& Pair : WeaponSlotContents)
	{
		if (Pair.Value.IsValid() && Pair.Value->GetItemActorClass() == ItemClass)
		{
			// 해당 슬롯의 현재 수량이 최대 슬롯 스택에 도달하지 않았으면 후보로 추가
			const int32 CurrQty = Pair.Value->GetQuantity();
			const int32 MaxSlotStack = Pair.Value->GetItemMaxSlotStackSize();
			if (CurrQty < MaxSlotStack)
			{
				OutPartialSlots.Add(Pair.Key);
			}
		}
	}
}

void UNAInventoryComponent::GatherEmptyInvenSlotsWithClass(UClass* ItemClass, TArray<FName>& OutEmptySlots) const
{
	if (!ItemClass) { ensure(false); return; }
	
	OutEmptySlots.Empty();
	if (ItemClass->IsChildOf<ANAWeapon>())
	{
		return;
	}
    
	for (const auto& Pair : InvenSlotContents)
	{
		if (!Pair.Value.IsValid())
		{
			OutEmptySlots.Add(Pair.Key);
		}
	}
}

void UNAInventoryComponent::GatherEmptyWeaponSlotsWithClass(UClass* ItemClass, TArray<FName>& OutEmptySlots) const
{
	if (!ItemClass) { ensure(false); return; }
	
	OutEmptySlots.Empty();
	if (!ItemClass->IsChildOf<ANAWeapon>())
	{
		return;
	}
    
	for (const auto& Pair : WeaponSlotContents)
	{
		if (!Pair.Value.IsValid())
		{
			OutEmptySlots.Add(Pair.Key);
		}
	}
}

int32 UNAInventoryComponent::ComputeDistributableAmount(UNAItemData* InputItem, const TArray<FName>& PartialSlots, const TArray<FName>& EmptySlots) const
{
	if (!InputItem) { return 0; }
	const int32 RequestedAmount = InputItem->GetQuantity();
	const int32 MaxInvHoldCount = InputItem->GetMaxInventoryHoldCount();

	// 1) Partial 슬롯에 이미 들어있는 수량
	int32 CurrTotalHold = 0;
	for (const FName& Slot : PartialSlots)
	{
		if (!InvenSlotContents[Slot].IsValid()) { continue; }
		CurrTotalHold += InvenSlotContents[Slot]->GetQuantity();
	}
	const int32 RemainingHold = MaxInvHoldCount == 0 ?
									RequestedAmount :
									FMath::Max(0, MaxInvHoldCount - CurrTotalHold);
	if (RemainingHold <= 0) { return 0; }

	// 2) Partial 슬롯들의 여유 공간 합산
	int32 PartialCapacity = 0;
	for (const FName& Slot : PartialSlots)
	{
		if (!InvenSlotContents[Slot].IsValid()) { continue; }
		PartialCapacity +=
			InvenSlotContents[Slot]->GetItemMaxSlotStackSize() - InvenSlotContents[Slot]->GetQuantity();
	}
	
	// 3) Empty 슬롯에서 만들 수 있는 신규 슬롯 여유 공간 합산
	int32 EmptyCapacity = 0;
	if (EmptySlots.Num() > 0)
	{
		const int32 MaxSlotStack = InputItem->GetItemMaxSlotStackSize();
		EmptyCapacity = EmptySlots.Num() * MaxSlotStack;
	}

	// 4) 최종으로 담을 수 있는 총 용량
	const int32 TotalSlotCapacity = PartialCapacity + EmptyCapacity;
	const int32 MaxAddableBySlot = FMath::Min(TotalSlotCapacity, RemainingHold);

	return FMath::Min(RequestedAmount, MaxAddableBySlot);
}

int32 UNAInventoryComponent::DistributeToSlots(UNAItemData* InputItem, int32 AmountToDistribute, const TArray<FName>& PartialSlots, const TArray<FName>& EmptySlots)
{
	 int32 Distributed = 0;

    // 1) Partial 슬롯 우선 채우기 (기존 슬롯을 최대치까지 채우며, 원본 수량만큼 차감)
    for (const FName& Slot : PartialSlots)
    {
        if (AmountToDistribute <= 0) { break; }
        UNAItemData* SlotItem = InvenSlotContents[Slot].Get();
        if (!SlotItem) { continue; }

        const int32 CurrQty   = SlotItem->GetQuantity();
        const int32 MaxSlot   = SlotItem->GetItemMaxSlotStackSize();
        const int32 CanAdd    = MaxSlot - CurrQty;
        if (CanAdd <= 0) { continue; }

        // 이 슬롯에 채울 수 있는 양
        const int32 ToAdd = FMath::Min(CanAdd, AmountToDistribute);

        // ─── 기존 슬롯에 수량 반영 ─────────────────────────────────
        SlotItem->SetQuantity(CurrQty + ToAdd);

        // ─── 원본 아이템 수량 차감 ─────────────────────────────────
        InputItem->SetQuantity(InputItem->GetQuantity() - ToAdd);

        AmountToDistribute -= ToAdd;
        Distributed       += ToAdd;
    }

    // 2) 남은 수량이 있으면 빈 슬롯에 채우기
    for (const FName& Slot : EmptySlots)
    {
        if (AmountToDistribute <= 0) { break; }
        if (InvenSlotContents[Slot].IsValid())
        {
            // 이미 다른 프로세스에서 채워졌거나, 슬롯에 데이터가 있는 경우 건너뜀
            continue;
        }

        // 현재 원본에 남은 수량
        const int32 RemainQty = InputItem->GetQuantity();
        if (RemainQty <= 0)
        {
            break;
        }

        // 한 슬롯에 채울 수 있는 최대치
        const int32 MaxSlot = InputItem->GetItemMaxSlotStackSize();

        // 기본적으로 이 슬롯에 채울 양
        int32 ToAdd = FMath::Min(MaxSlot, AmountToDistribute);

        // ─── 원본 수량 전부를 이 슬롯에 넣을 수 있다면, 복제 대신 원본을 이동 ───────────────────────
        if (ToAdd >= RemainQty)
        {
            // (1) 이 슬롯에 들어갈 양을 “원본 수량(RemainQty)”으로 조정
            ToAdd = RemainQty;

            // (2) 원본 데이터(InputItem)를 그대로 이 슬롯에 등록
            HandleAddNewItem(InputItem, Slot);

        	// ───── *중요* ─────  
        	// 이제 InputItem 객체는 “인벤토리 슬롯에 들어간 데이터”가 되므로, 
        	// InputItem->Quantity(==RemainQty) 값을 바꿀 필요가 없습니다.
        	
            Distributed       += ToAdd;
            AmountToDistribute -= ToAdd;

        	// 원본을 완전히 이동했으므로, 더 이상 빈 슬롯 순회를 하지 않고 루프 종료
            break;
        }
        // ─── 원본 수량 전부를 이 슬롯에 다 넣을 수 없다면, “복제본”을 만들어 슬롯에 일부 채우기 ──────────
        else
        {
            // (1) 원본 데이터(InputItem)의 복제본 생성
            UNAItemData* Duplicated = UNAItemEngineSubsystem::Get()->CreateItemDataCopy(InputItem);
            if (!Duplicated)
            {
                // 복제 실패 시, 이 슬롯도 채우지 않고 루프 종료
                break;
            }

            // (2) 복제본의 수량을 이 슬롯에 들어갈 양(ToAdd)으로 설정
            Duplicated->SetQuantity(ToAdd);

            // (3) 복제본을 슬롯에 등록
            HandleAddNewItem(Duplicated, Slot);

            // (4) 원본 수량에서ToAdd를 차감
            InputItem->SetQuantity(RemainQty - ToAdd);

            AmountToDistribute -= ToAdd;
            Distributed       += ToAdd;

            // 원본에 여전히 수량이 남아있으므로 루프는 계속
        }
    }

    return Distributed;
}

bool UNAInventoryComponent::IsValidForNonStackable(UNAItemData* InputItem, FString& OutFailReason) const
{
	if (!InputItem)
	{
		OutFailReason = TEXT("ItemToAdd was null.");
		return false;
	}
	// 1) 슬롯 1개에만 들어가는 아이템인지 확인
	const int32 MaxSlot = InputItem->GetItemMaxSlotStackSize();
	if (MaxSlot != 1)
	{
		OutFailReason = TEXT("비-스택형 아이템이 아닙니다.");
		return false;
	}
	// 2) 수량이 1인지
	if (InputItem->GetQuantity() != 1)
	{
		OutFailReason = TEXT("비-스택형 아이템의 수량은 1이어야 합니다.");
		return false;
	}
	// 3) 최대 소지 개수 검사
	const int32 MaxHold = InputItem->GetMaxInventoryHoldCount();
	if (MaxHold == 1 && HasItemOfClass(InputItem->GetItemActorClass()))
	{
		OutFailReason = TEXT("이미 동일한 아이템을 소지중입니다.");
		return false;
	}
	return true;
}

UNAItemData* UNAInventoryComponent::FindSameClassItem(const UClass* ItemClass) const
{
	if (!ItemClass) return nullptr;

	const TMap<FName, TWeakObjectPtr<UNAItemData>>* SlotMap = nullptr;

	if (ItemClass->IsChildOf<ANAWeapon>())
	{
		SlotMap = &WeaponSlotContents;
	}
	else
	{
		SlotMap = &InvenSlotContents;
	}

	int32 MinSlotNum = INT_MAX;
	UNAItemData* Result = nullptr;

	for (const auto& Pair : *SlotMap)
	{
		if (!Pair.Value.IsValid())
			continue;

		if (Pair.Value->GetItemActorClass()->IsChildOf(ItemClass))
		{
			int32 SlotNum = ExtractSlotNumber(Pair.Key);
			if (SlotNum < MinSlotNum)
			{
				MinSlotNum = SlotNum;
				Result = Pair.Value.Get();
			}
		}
	}

	return Result;
}

bool UNAInventoryComponent::FindSameClassItems( const UClass* ItemClass, TArray<UNAItemData*>& OutItems ) const
{
	OutItems.Empty();
	if (!ItemClass) return true;

	const TMap<FName, TWeakObjectPtr<UNAItemData>>* SlotMap = nullptr;

	if (ItemClass->IsChildOf<ANAWeapon>())
		SlotMap = &WeaponSlotContents;
	else
		SlotMap = &InvenSlotContents;

	// 임시로 (슬롯번호, 아이템) 쌍을 담음
	TArray<TPair<int32, UNAItemData*>> TempPairs;

	for (const auto& Pair : *SlotMap)
	{
		if (!Pair.Value.IsValid())
			continue;

		if (Pair.Value->GetItemActorClass()->IsChildOf(ItemClass))
		{
			int32 SlotNum = ExtractSlotNumber(Pair.Key);
			TempPairs.Emplace(SlotNum, Pair.Value.Get());
		}
	}

	// 슬롯 번호 오름차순 정렬
	TempPairs.Sort([](const TPair<int32, UNAItemData*>& A, const TPair<int32, UNAItemData*>& B) {
		return A.Key < B.Key;
	});

	// 정렬된 아이템만 추출
	for (const auto& Pair : TempPairs)
		OutItems.Add(Pair.Value);

	return OutItems.IsEmpty();
}

FName UNAInventoryComponent::FindSlotIDForItem(const UNAItemData* ItemToFind) const
{
	if (ItemToFind && ItemToFind->GetOwningInventory() == this)
	{
		if (ItemToFind->GetItemType() == EItemType::IT_Weapon)
		{
			for (const auto& Pair : WeaponSlotContents)
			{
				if (!Pair.Value.IsValid())
				{
					continue;
				}

				if (Pair.Value.Get() == ItemToFind)
				{
					return Pair.Key;
				}
			}
		}
		else
		{
			for (const auto& Pair : InvenSlotContents)
			{
				if (!Pair.Value.IsValid())
				{
					continue;
				}

				if (Pair.Value.Get() == ItemToFind)
				{
					return Pair.Key;
				}
			}	
		}
	}
	return NAME_None;
}

bool UNAInventoryComponent::HandleAddNewItem(UNAItemData* NewItemToAdd, const FName& SlotID)
{
	if (SlotID.IsNone() || !(InvenSlotContents.Contains(SlotID) || WeaponSlotContents.Contains(SlotID)))
	{
		ensure(false);
		return false;
	}
	UNAInventoryGameInstanceSubsystem* InvenSubsys = UNAInventoryGameInstanceSubsystem::Get(GetWorld());
	if (!InvenSubsys)
	{
		ensure(false);
		return false;
	}

	if (NewItemToAdd->GetItemType() == EItemType::IT_Weapon)
	{
		WeaponSlotContents[SlotID] = NewItemToAdd;
	}
	else
	{
		InvenSlotContents[SlotID] = NewItemToAdd;
	}
	NewItemToAdd->SetOwningInventory(this);
	InvenSubsys->AddItemToInventory(this, SlotID, NewItemToAdd);
	return true;
}

void UNAInventoryComponent::InitWidget()
{
	Super::InitWidget();

	if (IsValid(GetWidget()))
	{
		if (UNAInventoryWidget* InventoryWidget = Cast<UNAInventoryWidget>(GetWidget()))
		{
			InventoryWidget->InitInventoryWidget(this);
		}
	}
}

UNAInventoryWidget* UNAInventoryComponent::GetInventoryWidget() const
{
	return Cast<UNAInventoryWidget>(GetWidget());
}

UButton* UNAInventoryComponent::GetSlotButton(const FName& SlotID) const
{
	if (SlotID.IsNone()
		|| !InvenSlotContents.Contains(SlotID)
		|| !WeaponSlotContents.Contains(SlotID)
		/*|| !SlotButtons.Contains(SlotID)*/)
	{
		ensureAlwaysMsgf(false, TEXT("[UNAInventoryComponent::GetSlotButton]  유효하지 않은 슬롯ID: %s"), *SlotID.ToString());
		return nullptr;
	}

	// @TODO
	
	return /*SlotButtons[SlotID].Get();*/ nullptr;
}

UButton* UNAInventoryComponent::GetSlotButton(const UNAItemData* ItemData) const
{
	FName SlotID = FindSlotIDForItem(ItemData);
	if (!ItemData
		|| SlotID.IsNone()
		|| !IsValidSlotID(SlotID)
		/*|| !SlotButtons.Contains(SlotID)*/)
	{
		ensureAlwaysMsgf(false, TEXT("[UNAInventoryComponent::GetSlotButton]  유효하지 않은 아이템 데이터: %s"),
			*ItemData->GetItemID().ToString());
		return nullptr;
	}
	
	// @TODO
	
	return/* SlotButtons[SlotID].Get();*/ nullptr;
}

bool UNAInventoryComponent::IsInventoryWidgetVisible() const
{
	return IsVisible() && IsWidgetVisible();
}

void UNAInventoryComponent::ReleaseInventory()
{
	if (UNAInventoryWidget* InventoryWidget = Cast<UNAInventoryWidget>(GetWidget()))
	{
		InventoryWidget->ReleaseInventoryWidget();
	}
}

void UNAInventoryComponent::CollapseInventory()
{
	if (UNAInventoryWidget* InventoryWidget = Cast<UNAInventoryWidget>(GetWidget()))
	{
		InventoryWidget->CollapseInventoryWidget();
	}
}

void UNAInventoryComponent::SelectInventorySlotButton()
{
	if (IsInventoryWidgetVisible())
	{
		if (UNAInventoryWidget* InventoryWidget = Cast<UNAInventoryWidget>(GetWidget()))
		{
			InventoryWidget->SelectInventorySlotWidget();
		}
	}
}

void UNAInventoryComponent::RequestRedrawSingleSlot(UNAItemData* ItemData)
{
	if (!GetInventoryWidget()) return;
	
	if (!ItemData || ItemData->GetOwningInventory() != this) return;

	FName SlotID = FindSlotIDForItem(ItemData);
	if (SlotID.IsNone()) return;

	GetInventoryWidget()->RefreshSingleSlotWidget(SlotID, ItemData);
}

void UNAInventoryComponent::RequestRedrawNodesQuantity()
{
	if (!GetInventoryWidget()) return;
	
	GetInventoryWidget()->RefreshNodesQuantityWidget(FText::AsNumber(NodesCount));
}

void UNAInventoryComponent::RequestRedrawCreditsQuantity()
{
	if (!GetInventoryWidget()) return;

	GetInventoryWidget()->RefreshCreditsQuantityWidget(FText::AsNumber(CreditsCount));
}

