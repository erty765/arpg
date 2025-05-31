#pragma once

#include "Item/ItemActor/NAItemActor.h"
#include "NAPickableItemActor.generated.h"

UENUM(Blueprintable, meta=(Bitflags))
enum class EPickupMode : uint8
{
	PM_None        = 0 UMETA(DisplayName = "None"),

	PM_Inventory   = 1 << 0 UMETA(DisplayName = "Inventory"),
	PM_CarryOnly   = 1 << 1 UMETA(DisplayName = "Carry Only"),
	PM_AutoUse     = 1 << 2 UMETA(DisplayName = "Auto Use"),
	PM_Holdable    = 1 << 3 UMETA(DisplayName = "Holdable"),
};
ENUM_CLASS_FLAGS(EPickupMode)


UCLASS(Abstract)
class ARPG_API ANAPickableItemActor : public ANAItemActor
{
	GENERATED_BODY()

public:
	ANAPickableItemActor(const FObjectInitializer& ObjectInitializer);
	// UFUNCTION(BlueprintCallable, Category = "Pickable Item")
	// FORCEINLINE EPickupMode GetPickupMode() const
	// {
	// 	return 	PickupMode;
	// }

	//UFUNCTION(BlueprintCallable, Category = "Pickable Item")
	//void SetPickupMode(EPickupMode InPickupMode);

public:
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

protected:
	virtual void BeginPlay() override;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	virtual void BeginInteract_Implementation(AActor* Interactor) override;
	virtual void EndInteract_Implementation(AActor* Interactor) override;
	virtual void ExecuteInteract_Implementation(AActor* Interactor) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Pickable Item"
		/*,BlueprintGetter=GetPickupMode, BlueprintSetter=SetPickupMode*/)
	EPickupMode PickupMode = EPickupMode::PM_None;
};