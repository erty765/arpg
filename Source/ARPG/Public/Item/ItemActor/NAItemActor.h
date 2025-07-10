#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/NAInteractableInterface.h"
#include "Item/NAItemUseInterface.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "NAItemActor.generated.h"

class UTextRenderComponent;
class UNAMontageCombatComponent;
class UBillboardComponent;
class UMaterialInstanceConstant;

UENUM()
enum class EItemSubobjDirtyFlags : uint8
{
	ISDF_None	= (0x0),
	
	ISDF_CollisionShape		= (1<<0),
	ISDF_MeshType				= (1<<1),
};
ENUM_CLASS_FLAGS(EItemSubobjDirtyFlags)

UCLASS(Abstract)
class ARPG_API ANAItemActor : public AActor, public INAInteractableInterface, public INAItemUseInterface
{
	GENERATED_BODY()

	friend class UNAItemEngineSubsystem;
public:
	ANAItemActor(const FObjectInitializer& ObjectInitializer);
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PreRegisterAllComponents() override;
	virtual void PostRegisterAllComponents() override;
	virtual void PostActorCreated() override;
	virtual void PostNetReceive() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Item Actor")
	UNAItemData* GetItemData() const;

	UFUNCTION(BlueprintCallable, Category = "Item Actor")
	bool HasValidItemID() const;

	static void MigrateItemStateFromChildActor(ANAItemActor* SourceChildActor, ANAItemActor* TargetActor)
	{
		if ( UNAItemEngineSubsystem::Get() && SourceChildActor && TargetActor)
		{
			if (ensureAlwaysMsgf(SourceChildActor->IsChildActor() && SourceChildActor->HasValidItemID()
				&& !TargetActor->IsChildActor() && TargetActor->HasValidItemID(),
				TEXT(
					"[MigrateItemStateFromChildActor]  ")))
			{
				if (UNAItemEngineSubsystem::Get()->DestroyRuntimeItemData(TargetActor->ItemDataID))
				{
					TargetActor->ItemDataID = SourceChildActor->ItemDataID;
					if (SourceChildActor->InteractableInterfaceRef && TargetActor->InteractableInterfaceRef)
					{
						INAInteractableInterface::TransferInteractableStateToChildActor(
							SourceChildActor->InteractableInterfaceRef
							, TargetActor->InteractableInterfaceRef);
					}

					if (UChildActorComponent* ChildActorComponent =
						Cast<UChildActorComponent>(SourceChildActor->GetParentComponent()))
					{
						SourceChildActor->ItemDataID = NAME_None;
						ChildActorComponent->DestroyChildActor();
						ChildActorComponent->SetChildActorClass(nullptr);
					}
				}
			}
		}
	}

	static void MigrateItemStateToChildActor(ANAItemActor* SourceActor, ANAItemActor* TargetChildActor)
	{
		if (SourceActor && TargetChildActor)
		{
			if (ensureAlwaysMsgf(!SourceActor->IsChildActor() && SourceActor->HasValidItemID()
				&& TargetChildActor->IsChildActor() && !TargetChildActor->HasValidItemID(),
				TEXT(
					"[MigrateItemStateToChildActor]  ChildActorComponent에 의해 생성된 아이템 액터에 새로 생성된 아이템 데이터가 있었음")))
			{
				TargetChildActor->ItemDataID = SourceActor->ItemDataID;
				if (SourceActor->InteractableInterfaceRef && TargetChildActor->InteractableInterfaceRef)
				{
					INAInteractableInterface::TransferInteractableStateToChildActor(
						SourceActor->InteractableInterfaceRef
						, TargetChildActor->InteractableInterfaceRef);
				}

				SourceActor->ItemDataID = NAME_None;
				SourceActor->Destroy();
			}
		}
	}

	static void AssignItemDataToChildActor(UNAItemData* ItemData, ANAItemActor* TargetChildActor)
	{
		if (ItemData && !ItemData->GetItemID().IsNone() && TargetChildActor)
		{
			ensureAlwaysMsgf(TargetChildActor->IsChildActor() && !TargetChildActor->HasValidItemID(),
				TEXT(
					"[AssignItemDataToChildActor]  ChildActorComponent에 의해 생성된 아이템 액터에 새로 생성된 아이템 데이터가 있었음"));
			if (TargetChildActor->IsChildActor() && !TargetChildActor->GetItemData())
			{
				TargetChildActor->ItemDataID = ItemData->GetItemID();
				TargetChildActor->VerifyInteractableData();
			}
		}
	}

	TScriptInterface<INAInteractableInterface> GetInteractableInterface() const
	{
		return InteractableInterfaceRef;
	}

	virtual void ReleaseItemWidgetComponent();
	virtual void CollapseItemWidgetComponent();
	
protected:
	virtual EItemSubobjDirtyFlags GetDirtySubobjectFlags(const FNAItemBaseTableRow* MetaData) const;
	
	UFUNCTION()
	void OnActorBeginOverlap_Impl(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnActorEndOverlap_Impl(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void FinalizeAndDestroyAfterInventoryAdded_Impl(AActor* Interactor) {}

	void BroadcastInitialOverlapsOnTriggerSphere();

	void TransferItemWidgetToPopupBeforeDestroy() const;
	
	/** 기존 루트 컴포넌트를 제거하고, ItemCollision을 새로운 루트로 설정한 뒤, 기존 자식 컴포넌트들을 이관 */
	virtual void ReplaceRootWithItemCollisionIfNeeded();
	
	virtual void SetItemSubobjectsPhysics(const bool bEnable);
	
#if WITH_EDITOR || WITH_EDITORONLY_DATA
	void ReviseSubobjectsHierarchy();
#endif
private:
	void InitItemData();
	void VerifyInteractableData();
	void InitCheckIfChildActor();
	
	void InitItemSubobjectsAttachment();
	void InitItemSubobjectsCollision();

protected:
	friend struct FNAItemBaseTableRow;
	
	// Optional Subobject
	uint8 bNeedItemCollision :1 = true;
	
	// Optional Subobject
	uint8 bNeedItemMesh :1 = true;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Item Actor")
	bool bWasChildActor = false;
	
	UPROPERTY(Transient, NonTransactional, VisibleAnywhere, BlueprintReadOnly, Category="ItemActor")
	USceneComponent* StubRootComponent;
	
	UPROPERTY(Transient, NonTransactional, VisibleAnywhere, BlueprintReadOnly, Category="Item Actor | Collision Shape")
	UShapeComponent* ItemCollision;

	UPROPERTY(Transient, NonTransactional, VisibleAnywhere, Category = "Item Actor | Mesh")
	UMeshComponent* ItemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Item Actor | Static Mesh")
	TObjectPtr<class UGeometryCollection> ItemFractureCollection;
	
	UPROPERTY(VisibleAnywhere, Category = "Item Actor | Static Mesh")
	TObjectPtr<class UGeometryCollectionCache> ItemFractureCache;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item Actor | Trigger Sphere")
	TObjectPtr<class USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, Category = "Item Actor | Static Mesh")
	TObjectPtr<class UNAItemWidgetComponent> ItemWidgetComponent;

private:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Item Actor", meta = (AllowPrivateAccess = "true"))
	FName ItemDataID;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
public:
	virtual bool CanInteract_Implementation() const override;
	virtual void NotifyInteractableFocusBegin_Implementation(AActor* InteractableActor, AActor* InteractorActor) override;
	virtual void NotifyInteractableFocusEnd_Implementation(AActor* InteractableActor, AActor* InteractorActor) override;
	
	virtual bool IsOnInteract_Implementation() const override;
	
	virtual bool TryGetInteractableData(FNAInteractableData& OutData) const override final;
	virtual bool HasInteractionDelay() const override final;
	virtual float GetInteractionDelay() const override final;
	
	virtual bool IsAttachedAndPendingUse() const override;
	virtual void SetAttachedAndPendingUse(bool bNewState) override;

	virtual bool IsUnlimitedInteractable() const override final;
	virtual int32 GetInteractableCount() const override final;
	virtual void SetInteractableCount(int32 NewCount) override final;
	
	virtual bool CanPerformInteractionWith(AActor* Interactor) const override;

	virtual bool TryInteract_Implementation(AActor* Interactor) override;
	
protected:
	virtual bool BeginInteract_Implementation(AActor* Interactor) override;
	virtual bool ExecuteInteract_Implementation(AActor* Interactor) override;
	virtual bool EndInteract_Implementation(AActor* Interactor) override;

protected:
	/** 자기 자신(this)이 구현한 인터페이스를 보관 */
	UPROPERTY()
	TScriptInterface<INAInteractableInterface> InteractableInterfaceRef = nullptr;
};

UCLASS()
class ARPG_API ANAItemWidgetPopupActor final : public AActor
{
	GENERATED_BODY()
    
public:
	ANAItemWidgetPopupActor();

private:
	friend class ANAItemActor;
	/** 
	 * 외부에서 스폰 후 바로 호출할 초기화 함수
	 */
	void InitializePopup(UNAItemWidgetComponent* NewPopupWidgetComponent);
	
	/** 애니메이션 완료 시 호출될 함수 */
	UFUNCTION()
	void OnCollapseAnimationFinished();

private:
	/** Collapse 애니메이션이 담긴 위젯 컴포넌트 */
	UPROPERTY()
	UNAItemWidgetComponent* PopupWidgetComponent = nullptr;
};
