#pragma once

#include "Item/ItemActor/NAItemActor.h"
#include "NAPickableItemActor.generated.h"

/**
 *	@notice	아이템 상호작용 != 아이템 사용 (e.g. 물약 줍기 != 물약 사용) => 아이템 상호작용 입력 키 != 아이템 사용 입력 키
 *	캐릭터에서 아이템 상호작용 요청 -> 요청 받은 아이템에서 상호작용 처리(상호작용 내용은 아이템 종류 별로 다름)
 *	캐릭터에서 아이템 사용 요청 -> 요청 받은 아이템에서 사용 처리(용법은 아이템 종류 별로 다름)
 */

UENUM(Blueprintable, meta=(Bitflags))
enum class EPickupMode : uint8
{
	PM_None        = 0 UMETA(DisplayName = "None"),

	// 인벤토리에 "들어갈 수 있는" 아이템
	// PM_Holdable 플래그의 여부에 따라: 1. 바로 인벤토리로 물리적 이동(아이템 액터 파괴) / 2. 인벤토리에 적재되긴 하나 캐릭터의 메쉬에 어태치되어 사용 대기 상태가 됨
	PM_Inventory   = 1 << 0 UMETA(DisplayName = "Inventory"),
	
	// 줍기는 가능하지만 인벤토리에 못 들어가는 아이템. "들고 있기"만 가능. 캐릭터의 양쪽 손의 어태치 상황에 따라 줍기가 거부될 수 있음
	// @TODO: '들고 있기'는 전용 애니메이션이 필요할 수 있음. '들고 있기' 상태일 때, 상호작용 버튼의 동작: 내려놓기. '들고 있는 아이템 사용(들고 있기 상태일 때만 활성)' 전용 입력 키 추가 필요
	PM_CarryOnly   = 1 << 1 UMETA(DisplayName = "Carry Only"),

	// 줍자마자 자동 사용되는 아이템	(인벤토리 적재 가능 여부와 상관 없음)
	// 이 경우 아이템 사용 키는 활성되지 않음
	// PM_Inventory | PM_AutoUse -> 인벤토리에 아이템을 이동시키기 전에 자동 사용부터 시도함
	PM_AutoUse     = 1 << 2 UMETA(DisplayName = "Auto Use"),
};
ENUM_CLASS_FLAGS(EPickupMode)

// Pickable Item Actor: 아이템 상호작용이 "줍기"인 아이템 액터 (줍기 모드는 @see EPickupMode)
UCLASS(Abstract)
class ARPG_API ANAPickableItemActor : public ANAItemActor
{
	GENERATED_BODY()

public:
	ANAPickableItemActor(const FObjectInitializer& ObjectInitializer);
	
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	
//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	virtual bool CanInteract_Implementation() const override;

protected:
	virtual void SetInteractionPhysicsEnabled(const bool bEnabled) override;
	
//======================================================================================================================
// Pickable Item Actor Operations
//======================================================================================================================
public:
	UFUNCTION(BlueprintCallable, Category = "Pickable Item")
	FORCEINLINE EPickupMode GetPickupMode() const
	{
		return 	PickupMode;
	}
	
	// @return	자동 사용 때 소비한 수량
	//			-> -1이면 전부 소비 후 이 액터 destroy까지 끝냄
	//			-> 0이면 사용 실패, 0>이면 사용 성공
	UFUNCTION(BlueprintCallable, Category = "Pickable Item")
	int32 TryPerformAutoUse(AActor* User);

protected:
	// 오버라이딩 시 주의사항: 자동 사용의 결과로 아이템 수량에 변화가 있다면,
	//					   이 함수 내에서 SetQuantity를 호출하지 말고, 소비한 수량을 반환값으로 설정하기
	//					   수량 변화에 따른 후처리는 TryPerformAutoUse에서 실행되어야 함
	// @return	자동 사용 때 소비한 수량
	//			-> 0이면 사용 실패, 0>이면 사용 성공
	virtual int32 PerformAutoUse_Impl(AActor* User);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Pickable Item")
	EPickupMode PickupMode = EPickupMode::PM_None;
};