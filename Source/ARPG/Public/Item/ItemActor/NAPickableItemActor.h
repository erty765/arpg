#pragma once

#include "Item/ItemActor/NAItemActor.h"
#include "NAPickableItemActor.generated.h"

/**
 *	@notice	아이템 상호작용 != 아이템 사용 (e.g. 물약 줍기 != 물약 사용) => 아이템 상호작용 입력 키 != 아이템 사용 입력 키
 *	캐릭터에서 아이템 상호작용 요청 -> 요청 받은 아이템에서 상호작용 처리(상호작용 내용은 아이템 종류 별로 다름)
 *	캐릭터에서 아이템 사용 요청 -> 요청 받은 아이템에서 사용 처리(용법은 아이템 종류 별로 다름)
 */

// UENUM(Blueprintable)
// enum class EPickupMode : uint8
// {
// 	PM_Inventory  UMETA(DisplayName = "Inventory"),
// 	PM_CarryOnly  UMETA(DisplayName = "Carry Only"),
// 	PM_AutoUse    UMETA(DisplayName = "Auto Use"),
// };

UENUM(Blueprintable)
enum class EPickupMode : uint8
{
	PM_None       UMETA(DisplayName = "None"),

	// 인벤토리에 수납 가능한 아이템
	// 아이템 사용: 사용자의 명시적인 입력이 필요함
	PM_Inventory  UMETA(DisplayName = "Inventory"),
	
	// "들기"만 가능한 아이템.
	// '들기' 상태일 때, 상호작용 입력 발생
	//	→ '들기' 중인 아이템의 사용 가능 여부에 따라 아이템 사용 or '내려놓기'
	PM_CarryOnly  UMETA(DisplayName = "Carry Only"),

	// 줍자마자 자동 사용되는 아이템
	PM_AutoUse    UMETA(DisplayName = "Auto Use"),
};

// Pickable Item Actor: 아이템 상호작용이 "줍기"인 아이템 액터 (줍기 모드는 @see EPickupMode)
UCLASS(Abstract)
class ARPG_API ANAPickableItemActor : public ANAItemActor
{
	GENERATED_BODY()

public:
	ANAPickableItemActor(const FObjectInitializer& ObjectInitializer);
	
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

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