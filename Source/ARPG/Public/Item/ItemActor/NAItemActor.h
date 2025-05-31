#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/NAInteractableInterface.h"
#include "Item/ItemData/NAItemData.h"
#include "NAItemActor.generated.h"

UCLASS(Abstract)
class ARPG_API ANAItemActor : public AActor, public INAInteractableInterface
{
	GENERATED_BODY()

	friend class UNAItemEngineSubsystem;
public:
	ANAItemActor(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostLoadSubobjects( FObjectInstancingGraph* OuterInstanceGraph ) override;
	virtual void PostActorCreated() override;
	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
#if WITH_EDITOR
	// 블루프린트 에디터에서 프로퍼티 바뀔 때
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	void ExecuteItemPatch(UClass* ClassToPatch, const FNAItemBaseTableRow* PatchData, EItemMetaDirtyFlags PatchFlags);
#endif
	
protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Item Actor")
	const UNAItemData* GetItemData() const;
	
protected:
	// OnItemDataInitialized: BP 확장 가능
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnItemDataInitialized();
	virtual void OnItemDataInitialized_Implementation();

private:
	void InitItemData_Internal();
	void VerifyInteractableData_Internal();
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Item Actor | Root Shape")
	EItemRootShapeType ItemRootShapeType = EItemRootShapeType::IRT_None;
	UPROPERTY(Instanced, VisibleAnywhere, BlueprintReadOnly, Category="Item Actor | Root Shape")
	TObjectPtr<UShapeComponent> ItemRootShape;

	UPROPERTY(BlueprintReadOnly, Category="Item Actor | Mesh")
	EItemMeshType ItemMeshType = EItemMeshType::IMT_None;
	UPROPERTY(Instanced, VisibleAnywhere, Category = "Item Actor | Mesh")
	TObjectPtr<UMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Item Actor | Static Mesh")
	TObjectPtr<class UGeometryCollection> ItemFractureCollection;
	UPROPERTY(VisibleAnywhere, Category = "Item Actor | Static Mesh")
	TObjectPtr<class UGeometryCollectionCache> ItemFractureCache;

	UPROPERTY(Instanced, VisibleAnywhere, Category="Item Actor | Interaction Button")
	TObjectPtr<UBillboardComponent> ItemInteractionButton;

	UPROPERTY(Instanced, VisibleAnywhere, Category="Item Actor | Interaction Button")
	TObjectPtr<class UTextRenderComponent> ItemInteractionButtonText;

private:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Item Actor", meta = (AllowPrivateAccess = "true"))
	FName ItemDataID;

	UPROPERTY(Transient)
	uint8 bItemDataInitialized :1;
	
	static void MarkItemActorCDOSynchronized(TSubclassOf<ANAItemActor> ItemActorClass)
	{
		if (ItemActorClass)
		{
			if (ANAItemActor* CDO = Cast<ANAItemActor>(ItemActorClass.Get()->GetDefaultObject(false)))
			{
				CDO->bCDOSynchronizedWithMeta = true;
			}
		}
	}
	static bool IsItemActorCDOSynchronizedWithMeta(TSubclassOf<ANAItemActor> ItemActorClass)
	{
		if (ItemActorClass)
		{
			if (ANAItemActor* CDO = Cast<ANAItemActor>(ItemActorClass.Get()->GetDefaultObject(false)))
			{
				return CDO->bCDOSynchronizedWithMeta;
			}
		}
		return false;
	}
	// 이 변수 건들면 안됨
	UPROPERTY(Transient)
	uint8 bCDOSynchronizedWithMeta : 1 = false;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	virtual FNAInteractableData GetInteractableData_Implementation() const override;
	virtual bool GetInteractableData_Internal(FNAInteractableData& OutInteractableData) const override;
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
	TScriptInterface<INAInteractableInterface> InteractableInterfaceRef = nullptr;
	
};