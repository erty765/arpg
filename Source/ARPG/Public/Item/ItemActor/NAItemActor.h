#pragma once

#include "GameFramework/Actor.h"
#include "Item/GameInstance/NAItemGameInstanceSubsystem.h"
#include "Interaction/NAInteractableInterface.h"
#include "Item/ItemManagerStatics.h"
#include "NAItemActor.generated.h"


/*#define IMPLEMENT_MACROHOOK_InitItemData(RowStructType)\
	static_assert(TIsDerivedFrom<RowStructType, FNAItemBaseTableRow>::IsDerived,\
	#RowStructType "은 반드시 FNAItemBaseTableRow 파생 타입이어야만.");\
protected:\
	virtual void InitItemData_MacroHook() override\
	{\
		bIsItemDataInitialized = CreateItemDataIfUnset<RowStructType>();\
	}
*/
#define IMPLEMENT_MACROHOOK_GetInitInteractableDataParams(Type, /*Script,*/Duration, Quantity)\
protected:\
virtual void GetInitInteractableDataParams_MacroHook(\
		ENAInteractableType& OutInteractableType,\
		FString&			 OutInteractableName,\
		/*FString				OutInteractionScript,*/\
		float&				 OutInteractableDuration,\
		int32&				 OutQuantity) const override\
{\
	OutInteractableType		= Type;\
	OutInteractableName		= GetItemData()->GetItemMetaDataStruct<FNAItemBaseTableRow>()->TextData.InteractionText.ToString();\
	/*OutInteractionScript		= Script*/\
	OutInteractableDuration = Duration;\
	OutQuantity				= Quantity;\
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemActorInitialized, ANAItemActor*, InitializedItemActor);

UCLASS()
class ARPG_API ANAItemActor : public AActor, public INAInteractableInterface
{
	GENERATED_BODY()

public:
	ANAItemActor(const FObjectInitializer& ObjectInitializer);
	
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Item Instance")
	FORCEINLINE UNAItemData* GetItemData() const
	{
		return ItemData.Get();
	}

protected:
	// 파생 클래스에서 InitItemData_Impl 오버라이딩 시, UItemGameInstanceSubsystem::CreateItemDataByActor 호출 간소화를 위한 템플릿 래퍼 함수
	// @param	ItemDTRow_T:	해당 클래스 전용의 아이템 DT Row 타입
	// @return	해당 객체의 멤버 ItemData에 유효한 값이 할당되었는지 여부
	template<typename ItemDTRow_T = FNAItemBaseTableRow>
		requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
	bool CreateItemDataIfUnset() noexcept;

	// 파생 클래스가 만약 전용 DT Row 타입을 써야한다면 이 함수를 오버라이딩 하셍요
	// 오버라이딩이 필요하다면, 파생 클래스 선언 부 안에 IMPLEMENT_MACROHOOK_InitItemData 매크로 쓰셈
	// e.g.) IMPLEMENT_ITEM_DATA(FNAWeaponTableRow)
	//virtual void InitItemData_MacroHook();

	// OnItemDataInitialized: BP 확장 가능
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnItemDataInitialized();
	virtual void OnItemDataInitialized_Implementation() {}

	// 파생 클래스 오버라이딩 전용
	virtual void VerifyInteractableData_Impl() {}

	// 파생 클래스 오버라이딩 전용
	// @parm	InDataTableRowHandle: ANAItemActor::InitItemActor_Internal에서 유효성 검사 실행한 뒤 전달됨
	virtual void InitItemActor_Impl();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnItemActorInitialized();
	virtual void OnItemActorInitialized_Implementation();
	
	virtual void BeginPlay() override;

protected:
	UFUNCTION(BlueprintCallable)
	void InitItemActor_Internal();
	
private:
	void InitItemData_Internal();
	
	void VerifyInteractableData_Internal();

public:
	UPROPERTY(BlueprintAssignable, Category = "Item Instance")
	FOnItemActorInitialized OnItemActorInitializedDelegate;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Instance | Root Shape")
	TSubclassOf<UShapeComponent> ItemRootShapeClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item Instance | Root Shape")
	TObjectPtr<class UShapeComponent> ItemRootShape = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Item Instance | Mesh")
	TSubclassOf<UMeshComponent> ItemMeshClass;

	UPROPERTY(VisibleAnywhere, Category = "Item Instance | Mesh")
	TObjectPtr<class UMeshComponent> ItemMesh;

	// UPROPERTY(VisibleAnywhere, Category = "Item Instance | Static Mesh")
	// TObjectPtr<class UGeometryCollection> ItemFractureCollection = nullptr;
	//
	// UPROPERTY(VisibleAnywhere, Category = "Item Instance | Static Mesh")
	// TObjectPtr<class UGeometryCollectionCache> ItemFractureCache = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category = "Item Instance | Static Mesh")
	TSubclassOf<UAnimInstance> ItemAnimClass;
	
	uint8 bIsItemDataInitialized :1 = false;
	uint8 bWTF :1 = false;
	uint8 bHasShapeRoot : 1 = false;

	UPROPERTY(VisibleAnywhere, Category="Item Instance | Interaction Button")
	TObjectPtr<class UBillboardComponent> ItemInteractionButton;
	UPROPERTY(VisibleAnywhere, Category="Item Instance | Interaction Button")
	TObjectPtr<class UTextRenderComponent> ItemInteractionButtonText;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Instance", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<UNAItemData> ItemData = nullptr;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	virtual FNAInteractableData GetInteractableData_Implementation() const override;
	virtual const FNAInteractableData& GetInteractableData_Internal() const override;
	virtual void SetInteractableData_Implementation(const FNAInteractableData& NewInteractableData) override;
	virtual bool CanUseRootAsTriggerShape_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;
	virtual void NotifyInteractableFocusBegin_Implementation(AActor* InteractableActor, AActor* InteractorActor) override;
	virtual void NotifyInteractableFocusEnd_Implementation(AActor* InteractableActor, AActor* InteractorActor) override;

	virtual void BeginInteract_Implementation(AActor* Interactor) override;
	virtual void EndInteract_Implementation(AActor* Interactor) override;
	virtual void ExecuteInteract_Implementation(AActor* Interactor) override;

	virtual bool IsOnInteract_Implementation() const override;

protected:
	/** 자기 자신(this)이 구현한 인터페이스를 보관 */
	UPROPERTY()
	TScriptInterface<INAInteractableInterface> InteractableInterfaceRef;
	
	// INAInteractableInterface 메서드 x, ANAItemActor의 파생 클래스에서 구현해야하는 매크로 훅 함수
	// 오버라이딩이 필요하다면, 파생 클래스 선언 부 안에 IMPLEMENT_MACROHOOK_GetInitInteractableDataParams 매크로 쓰셈
	virtual void GetInitInteractableDataParams_MacroHook(
		ENAInteractableType& OutInteractableType,
		FString&			 OutInteractableName,
		/*FString&			 OutInteractableScript,*/
		float&				 OutInteractableDuration,
		int32&				 OutQuantity) const;
	
private:
	virtual void SetInteractableDataToBaseline_Implementation(
		ENAInteractableType InInteractableType,
		const FString&				InInteractionName,
		/*const FString&				InInteractionScript,*/
		float				InInteractionDuration,
		int32				InQuantity) override final;
};

class UNAItemGameInstanceSubsystem;

template<typename ItemDTRow_T>
	requires TIsDerivedFrom<ItemDTRow_T, FNAItemBaseTableRow>::IsDerived
inline bool ANAItemActor::CreateItemDataIfUnset() noexcept
{
	if (ItemData.IsExplicitlyNull())
	{
		if (TWeakObjectPtr<UNAItemData> NewItemData = FItemManagerStatics::Get(GetWorld()).CreateItemDataByActor(this);NewItemData.IsValid())
		{
			ItemData = NewItemData.Get();
			return true;
		}
	} // sh1t
	else {
		UE_LOG(LogTemp, Warning, TEXT("[ANAItemActor::CreateItemDataIfUnset]  이미 초기화된 ItemData."));
	}

	return false;
}
