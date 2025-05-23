
#include "Item/ItemActor/NAItemActor.h"

#include "Components/ShapeComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/NAInteractionComponent.h"
#include "Item/ItemActor/NAPickableItemActor.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"


ANAItemActor::ANAItemActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void ANAItemActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (HasAnyFlags(RF_ClassDefaultObject)) { return; }
	
}

void ANAItemActor::InitItemData_Internal()
{
	//InitItemData_MacroHook();
	bIsItemDataInitialized = CreateItemDataIfUnset<FNAItemBaseTableRow>();
	if (bIsItemDataInitialized)
	{
		OnItemDataInitialized();
	}
}

// void ANAItemActor::InitItemData_MacroHook()
// {
// 	UE_LOG(LogTemp, Warning, TEXT("[%s::InitItemData_MacroHook]  오버라이딩 없음. FNAItemBaseTableRow로 ItemData 초기화"), *GetClass()->GetName());
//
// 	bIsItemDataInitialized = CreateItemDataIfUnset<FNAItemBaseTableRow>();
// }

void ANAItemActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (HasAnyFlags(RF_ClassDefaultObject)) { return; }
}

void ANAItemActor::VerifyInteractableData_Internal()
{
	// 이 액터가 UNAInteractableInterface 인터페이스를 구현했다면 this를 할당
	if (bIsItemDataInitialized && GetClass()->ImplementsInterface(UNAInteractableInterface::StaticClass()))
	{
		if (!InteractableInterfaceRef)
		{
			InteractableInterfaceRef = this;
		}
		
		if (!CheckInteractableEdit(InteractableInterfaceRef.GetInterface()->Execute_GetInteractableData(this)))
		{
			if (!ItemData->ItemMetaDataHandle.IsNull())
			{
				ENAInteractableType Type;
				FString				Name, Script;
				float				Duration;
				int32				Quantity;
				GetInitInteractableDataParams_MacroHook(Type,Name/*, Script*/, Duration, Quantity);
				SetInteractableDataToBaseline(Type, Name/*, Script*/, Duration, Quantity);
			}
		}

		// 파생 클래스에서 확장한 로직
		VerifyInteractableData_Impl();
	}
	else
	{
		// 로그
	}
}

void ANAItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (HasAnyFlags(RF_ClassDefaultObject)) { return; }
	
	if (!ItemData.IsValid())
	{
		InitItemData_Internal();
		//ensureAlwaysMsgf(bIsItemDataInitialized, TEXT("[ANAItemActor::OnConstruction]  아직까지도 Item Data가 초기화되지 않았다고 어째서야"));
		//return;
		InitItemActor_Internal();
	}

	// if (!bWTF)
	// {
	// 	
	// }
}

void ANAItemActor::InitItemActor_Internal()
{
	// if (!bIsItemDataInitialized || ItemData->ItemMetaDataHandle.IsNull())
	// {
	// 	//ensureAlwaysMsgf(false, TEXT("[ANAItemActor::InitItemActor_Internal]  InDataTableRowHandle이 Null이었음. Item Instance 초기화 실패"));
	// 	//return;
	// 	InitItemData_Internal();
	// }
	// if (bIsItemDataInitialized)
	if (ItemData.IsValid())
	{
		InitItemActor_Impl();
		OnItemActorInitialized();
	}
	
}

void ANAItemActor::OnItemActorInitialized_Implementation()
{
	VerifyInteractableData_Internal();
}


void ANAItemActor::InitItemActor_Impl()
{
	const FNAItemBaseTableRow* ItemDataStruct = ItemData->GetItemMetaDataStruct<FNAItemBaseTableRow>();
	if (!ItemDataStruct)
	{
		ensure(false);
		return;
	}

	EObjectFlags SubobjectFlags = GetMaskedFlags(RF_PropagateToSubObjects) | RF_DefaultSubObject;

	float RootShapeHeight = 0.f;
	if (!ItemRootShape)
	{
		if (ItemDataStruct->RootShapeType != EItemRootShapeType::IRT_None)
		{
			RootComponent->DestroyComponent();
			bHasShapeRoot = true;
		}
		
		USphereComponent* NewRootSphere = nullptr;
		UBoxComponent* NewRootBox = nullptr;
		UCapsuleComponent* NewRootCapsule = nullptr;
		
		switch (ItemDataStruct->RootShapeType)
		{
		// ANAItemActor 루트 컴포넌트에 콜리전 활성화가 필요없다면 기본으로 생성된 USceneComponent를 루트 컴포넌트로 사용 
		case EItemRootShapeType::IRT_None:
			//UE_LOG(LogTemp, Warning, TEXT("[%s::InitItemActor_Impl]  ItemData의 RootShapeType이 None이었음. 기본값 Sphere 사용."), *GetClass()->GetName());
			break;
			
		case EItemRootShapeType::IRT_Sphere:
			ItemRootShapeClass = USphereComponent::StaticClass();
			NewRootSphere = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), TEXT("ItemRootShape"), SubobjectFlags);
			if (NewRootSphere)
			{
				ItemRootShape = NewRootSphere;
				SetRootComponent(ItemRootShape);
			
				NewRootSphere->SetRelativeTransform(ItemDataStruct->RootShapeTransform);
				NewRootSphere->SetSphereRadius(ItemDataStruct->RootSphereRadius);
				RootShapeHeight = NewRootSphere->GetScaledSphereRadius() * 2.f;
				// 콜리전 쿼리 및 채널 등등
				NewRootSphere->SetGenerateOverlapEvents(true);
				//NewRootSphere->SetCollisionEnabled();
			}
			break;
		case EItemRootShapeType::IRT_Box:
			ItemRootShapeClass = UBoxComponent::StaticClass();
			NewRootBox = NewObject<UBoxComponent>(this, UBoxComponent::StaticClass(), TEXT("ItemRootShape"), SubobjectFlags);
			if (NewRootBox)
			{
				ItemRootShape = NewRootBox;
				SetRootComponent(ItemRootShape);
			
				NewRootBox->SetRelativeTransform(ItemDataStruct->RootShapeTransform);
				NewRootBox->SetBoxExtent(ItemDataStruct->RootBoxExtent);
				RootShapeHeight = NewRootBox->GetScaledBoxExtent().Z;
				// 콜리전 쿼리 및 채널 등등
				NewRootBox->SetGenerateOverlapEvents(true);
				//NewRootBox->SetCollisionEnabled();
			}
			break;
		case EItemRootShapeType::IRT_Capsule:
			ItemRootShapeClass = UCapsuleComponent::StaticClass();
			NewRootCapsule = NewObject<UCapsuleComponent>(this, UCapsuleComponent::StaticClass(), TEXT("ItemRootShape"), SubobjectFlags);
			if (NewRootCapsule)
			{
				ItemRootShape = NewRootCapsule;
				SetRootComponent(ItemRootShape);

				NewRootCapsule->SetRelativeTransform(ItemDataStruct->RootShapeTransform);
				NewRootCapsule->SetCapsuleSize(ItemDataStruct->RootCapsuleRadius,  ItemDataStruct->RootCapsuleHalfHeight);
				RootShapeHeight = NewRootCapsule->GetScaledCapsuleHalfHeight() * 2.f;
				// 콜리전 쿼리 및 채널 등등
				NewRootCapsule->SetGenerateOverlapEvents(true);
				//NewRootCapsule->SetCollisionEnabled();
			}
			break;
		}
	}

	if (RootComponent && !ItemMesh)
	{
		UStaticMeshComponent* NewStaticMeshComp = nullptr;
		USkeletalMeshComponent* NewSkeletalMeshComp = nullptr;
		
		switch (ItemDataStruct->MeshType)
		{
		case EItemMeshType::IMT_None:
			UE_LOG(LogTemp, Warning, TEXT("[%s::InitItemActor_Impl]  ItemData의 MeshType이 None이었음. 기본값 Static 사용."), *GetClass()->GetName());

		case EItemMeshType::IMT_Static:
			ItemMeshClass = UStaticMeshComponent::StaticClass();
			NewStaticMeshComp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), TEXT("ItemMesh"), SubobjectFlags);
			if (NewStaticMeshComp)
			{
				ItemMesh = NewStaticMeshComp;
				ItemMesh->AttachToComponent(ItemRootShape, FAttachmentTransformRules::KeepRelativeTransform);

				NewStaticMeshComp->SetStaticMesh(ItemDataStruct->StaticMeshAssetData.StaticMesh);
				NewStaticMeshComp->SetRelativeTransform(ItemDataStruct->StaticMeshAssetData.MeshTransform);
				// 콜리전 그 외 등등
			}
			break;
		case EItemMeshType::IMT_Skeletal:
			ItemMeshClass = USkeletalMeshComponent::StaticClass();
			NewSkeletalMeshComp = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass(), TEXT("ItemMesh"), SubobjectFlags);
			if (NewSkeletalMeshComp)
			{
				ItemMesh = NewSkeletalMeshComp;
				ItemMesh->AttachToComponent(ItemRootShape, FAttachmentTransformRules::KeepRelativeTransform);

				NewSkeletalMeshComp->SetSkeletalMesh(ItemDataStruct->SkeletalMeshAssetData.SkeletalMesh);
				NewSkeletalMeshComp->SetRelativeTransform(ItemDataStruct->SkeletalMeshAssetData.MeshTransform);
				ItemAnimClass = ItemDataStruct->SkeletalMeshAssetData.AnimClass;
				if (ItemAnimClass)
				{
					NewSkeletalMeshComp->SetAnimClass(ItemAnimClass);
				}
				
				// 콜리전 그 외 등등
			}
			break;
		}
	}

	if (!ItemInteractionButton)
	{
		ItemInteractionButton = NewObject<UBillboardComponent>(this, UBillboardComponent::StaticClass(), TEXT("ItemInteractionButton"), SubobjectFlags);
		ItemInteractionButton->AttachToComponent(ItemRootShape, FAttachmentTransformRules::KeepRelativeTransform);
		ItemInteractionButton->SetSprite(ItemDataStruct->IconAssetData.Icon);
		ItemInteractionButton->SetRelativeTransform(ItemDataStruct->IconAssetData.IconTransform);
		if (!FMath::IsNearlyZero(RootShapeHeight))
		{
			FVector ButtonLoc = ItemInteractionButton->GetRelativeLocation();
			ButtonLoc.Z += RootShapeHeight;
			ItemInteractionButton->SetRelativeLocation(ButtonLoc);
		}
	}

	if (!ItemInteractionButtonText)
	{
		ItemInteractionButtonText = NewObject<UTextRenderComponent>(this, UTextRenderComponent::StaticClass(), TEXT("ItemInteractionButtonText"), SubobjectFlags);
		ItemInteractionButtonText->AttachToComponent(ItemInteractionButton, FAttachmentTransformRules::KeepRelativeTransform);
		ItemInteractionButtonText->SetRelativeTransform(ItemDataStruct->IconAssetData.IconTransform);
		ItemInteractionButtonText->SetText(ItemDataStruct->TextData.InteractionText);
	}

	bWTF = true;
	RegisterAllComponents();
}

void ANAItemActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (!bWTF)
	{
		InitItemData_Internal();
		ensureAlwaysMsgf(bIsItemDataInitialized, TEXT("[ANAItemActor::OnConstruction]  아직까지도 Item Data가 초기화되지 않았다고 어째서야"));
		if (bIsItemDataInitialized)
		{
			InitItemActor_Internal();
		}
	}
	if (Execute_CanUseRootAsTriggerShape(this))
	{
		OnActorBeginOverlap.AddDynamic(this, &ThisClass::NotifyInteractableFocusBegin);
		OnActorEndOverlap.AddDynamic(this, &ThisClass::NotifyInteractableFocusEnd);
	}
	
}

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================

FNAInteractableData ANAItemActor::GetInteractableData_Implementation() const
{
	return GetInteractableData_Internal();
}

const FNAInteractableData& ANAItemActor::GetInteractableData_Internal() const
{
	const FNAInteractableData* InteractableDataRef = nullptr;
	if (ItemData.IsValid())
	{
		const FNAItemBaseTableRow* ItemMetaData = ItemData->GetItemMetaDataStruct<FNAItemBaseTableRow>();
		if (ItemMetaData)
		{
			InteractableDataRef = TryGetInteractableData(ItemMetaData);
		}
	}
	
	return *InteractableDataRef;
}

void ANAItemActor::SetInteractableData_Implementation(const FNAInteractableData& NewInteractableData)
{
	if (ItemData.IsValid())
	{
		FNAItemBaseTableRow* ItemMetaData = ItemData->ItemMetaDataHandle.GetRow<FNAItemBaseTableRow>(ItemData->ItemMetaDataHandle.RowName.ToString());
		if (ItemMetaData)
		{
			TrySetInteractableData(ItemMetaData, NewInteractableData);
		}
	}
}

bool ANAItemActor::CanUseRootAsTriggerShape_Implementation() const
{
	return bHasShapeRoot;
}

bool ANAItemActor::CanInteract_Implementation() const
{
	return bIsFocused && Execute_CanUseRootAsTriggerShape(this) && Execute_GetInteractableData(this).InteractingCharacter.IsValid();
}

void ANAItemActor::NotifyInteractableFocusBegin_Implementation(AActor* InteractableActor, AActor* InteractorActor)
{
	bIsFocused = true;
	
	if (Execute_CanInteract(this))
	{
		if (UNAInteractionComponent* InteractionComp = TryGetInteractionComponent(InteractorActor))
		{
			//InteractionComp->OnInteractableFound(this);
			const bool bSucceed = InteractionComp->OnInteractableFound(this);
			// @TODO: ANAItemActor 쪽에서 '상호작용 버튼 위젯' release?
		}
	}
}

void ANAItemActor::NotifyInteractableFocusEnd_Implementation(AActor* InteractableActor, AActor* InteractorActor)
{
	bIsFocused = false;
	
	if (Execute_CanInteract(this))
	{
		if (UNAInteractionComponent* InteractionComp = TryGetInteractionComponent(InteractorActor))
		{
			//InteractionComp->OnInteractableLost(this);
			const bool bSucceed = InteractionComp->OnInteractableLost(this);
			// @TODO: ANAItemActor 쪽에서 '상호작용 버튼 위젯' collapse?
		}
	}
}

void ANAItemActor::BeginInteract_Implementation(AActor* InteractorActor)
{
	//if (UNAInteractionComponent* InteractionComp = TryGetInteractionComponent(InteractorActor))
	//{
	bIsOnInteract = true;

	// @TODO: 상호작용 시작 시 필요한 로직이 있으면 여기에 추가, 상호작용 시작을 알리는 이벤트라고 생각하면 됨
	//}
}

void ANAItemActor::EndInteract_Implementation(AActor* InteractorActor)
{
	//if (UNAInteractionComponent* InteractionComp = TryGetInteractionComponent(InteractorActor))
	//{
	bIsOnInteract = false;

	// @TODO: 상호작용 종료 시 필요한 로직이 있으면 여기에 추가, 상호작용 종료를 알리는 이벤트라고 생각하면 됨
	//}
}

void ANAItemActor::ExecuteInteract_Implementation(AActor* InteractorActor)
{
	ensureAlwaysMsgf(bIsOnInteract, TEXT("[INAInteractableInterface::ExecuteInteract_Implementation]  bIsOnInteract이 false였음"));
	
	//if (UNAInteractionComponent* InteractionComp = TryGetInteractionComponent(InteractorActor))
	//{

	// @TODO: 상호작용 실행에 필요한 로직이 있으면 여기에 추가
	//}
	
}

void ANAItemActor::GetInitInteractableDataParams_MacroHook(ENAInteractableType& OutInteractableType, FString& OutInteractableName,
	/*FString& OutInteractableScript,*/ float& OutInteractableDuration, int32& OutQuantity) const
{
	UE_LOG(LogTemp, Warning, TEXT("[%s::GetInitInteractableDataParams_MacroHook]  오버라이딩 없음. ANAItemActor의 기본값으로 초기화"), *GetClass()->GetName());
	
	OutInteractableType		= ENAInteractableType::None;
	OutInteractableName		= ItemData->GetItemMetaDataStruct<FNAItemBaseTableRow>()->TextData.InteractionText.ToString();
	/*OutInteractableScript	= TEXT("");*/
	OutInteractableDuration = 0.f;
	OutQuantity				= 0;
}

void ANAItemActor::SetInteractableDataToBaseline_Implementation(ENAInteractableType InInteractableType,
                                                          const FString& InInteractionName, /*const FString& InInteractionScript,*/ float InInteractionDuration, int32 InQuantity)
{
	if (!this->GetClass()->IsChildOf<class ANAPickableItemActor>())
	{
		InQuantity = FMath::Clamp(InQuantity, 0, 1);
	}
	
	FNAInteractableData InteractableDataBaseline;
	InteractableDataBaseline.InteractableType = InInteractableType;
	InteractableDataBaseline.InteractionName = FText::FromString(InInteractionName);
	//InteractableDataBaseline.InteractionScript = FText::FromString(InInteractionScript);
	InteractableDataBaseline.InteractionDuration = InInteractionDuration;
	InteractableDataBaseline.Quantity = InQuantity;
	MarkBaselineModified(InteractableDataBaseline);

	Execute_SetInteractableData(this, InteractableDataBaseline);
}

bool ANAItemActor::IsOnInteract_Implementation() const
{
	return bIsOnInteract;
}