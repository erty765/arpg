
#include "Item/ItemActor/NAItemActor.h"

#include "NACharacter.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/NAInteractionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemWidget/NAItemWidget.h"
#include "Misc/NALogCategory.h"

#if WITH_EDITOR
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#endif

ANAItemActor::ANAItemActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	{
		// if (HasAnyFlags(RF_ClassDefaultObject))
		// {
		// 	if (!GetClass()->HasAllClassFlags(CLASS_CompiledFromBlueprint))
		// 	{
		// 		UE_LOG(NAItem, Log, TEXT("[ANAItemActor] C++ CDO (%s)"), *GetNameSafe(this));
		// 	}
		// 	else
		// 	{
		// 		UE_LOG(NAItem, Log, TEXT("[ANAItemActor] BP CDO (%s)"), *GetNameSafe(this));
		// 	}
		// }
		// else
		// {
		// 	if (!GetClass()->HasAllClassFlags(CLASS_CompiledFromBlueprint))
		// 	{
		// 		UE_LOG(NAItem, Log, TEXT("[ANAItemActor] C++ 인스턴스 (%s)"), *GetNameSafe(this));
		// 	}
		// 	else
		// 	{
		// 		UE_LOG(NAItem, Log, TEXT("[ANAItemActor] BP 인스턴스 (%s)"), *GetNameSafe(this));
		// 	}
		// }
	}

	StubRootComponent = CreateDefaultSubobject<USceneComponent>( "StubRootComponent");
	SetRootComponent( StubRootComponent );

	if (UNAItemEngineSubsystem* ItemEngineSubsystem = UNAItemEngineSubsystem::Get())
	{
		if (const FNAItemBaseTableRow* MetaData = ItemEngineSubsystem->GetItemMetaDataByClass(GetClass()))
		{
			switch (MetaData->CollisionShape)
			{
			case EItemCollisionShape::ICS_Sphere:
				ItemCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ItemCollision(Sphere)"), true);
				break;
			case EItemCollisionShape::ICS_Box:
				ItemCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ItemCollision(Box)"), true);
				break;
			case EItemCollisionShape::ICS_Capsule:
				ItemCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ItemCollision(Capsule)"), true);
				break;
			default:
				break;
			}
			if (ItemCollision)
			{
				if (USphereComponent* SphereCollision = Cast<USphereComponent>(ItemCollision))
				{
					SphereCollision->SetSphereRadius(MetaData->CollisionSphereRadius);
				}
				else if (UBoxComponent* BoxCollision = Cast<UBoxComponent>(ItemCollision))
				{
					BoxCollision->SetBoxExtent(MetaData->CollisionBoxExtent);
				}
				else if (UCapsuleComponent* CapsuleCollision = Cast<UCapsuleComponent>(ItemCollision))
				{
					CapsuleCollision->SetCapsuleSize(
						MetaData->CollisionCapsuleSize.X, MetaData->CollisionCapsuleSize.Y);
				}
				ItemCollision->SetRelativeTransform(FTransform::Identity);
			}
			
			switch (MetaData->MeshType)
			{
			case EItemMeshType::IMT_Static:
				ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh(Static)"), true);
				break;
			case EItemMeshType::IMT_Skeletal:
				ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh(Skeletal)"), true);
				break;
			default:
				break;
			}

			if (ItemMesh)
			{
				if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ItemMesh))
				{
					StaticMeshComp->SetStaticMesh(MetaData->StaticMeshAssetData.StaticMesh);
					ItemFractureCollection = MetaData->StaticMeshAssetData.FractureCollection;
					ItemFractureCache = MetaData->StaticMeshAssetData.FractureCache;
				}
				else if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(ItemMesh))
				{
					SkeletalMeshComp->SetSkeletalMesh(MetaData->SkeletalMeshAssetData.SkeletalMesh);
					SkeletalMeshComp->SetAnimClass(MetaData->SkeletalMeshAssetData.AnimClass);
				}
				ItemMesh->SetRelativeTransform(MetaData->MeshTransform);
			}
		}
	}
	
	TriggerSphere = CreateDefaultSubobject<USphereComponent>("TriggerSphere");

	ItemWidgetComponent
		= CreateOptionalDefaultSubobject<UNAItemWidgetComponent>(TEXT("ItemWidgetComponent"));
	
	bAlwaysRelevant = true;
	bReplicates = true;
	SetReplicateMovement(true);
	
	ItemDataID = NAME_None;

	if (ItemCollision)
	{
		ItemCollision->SetupAttachment(GetRootComponent());
	}
	if (ItemMesh)
	{
		ItemMesh->SetupAttachment(GetRootComponent());
	}
	if (TriggerSphere)
	{
		TriggerSphere->SetupAttachment(GetRootComponent());
	}
	if (ItemWidgetComponent)
	{
		ItemWidgetComponent->SetupAttachment(GetRootComponent());
	}
	
	InitItemSubobjectsPhysics();
}

void ANAItemActor::InitItemSubobjectsPhysics()
{
	// 콜리전, 피직스 등등 설정 여기에
	if (ItemCollision)
	{
		ItemCollision->SetIsReplicated(true);
		ItemCollision->SetNetAddressable();
		ItemCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		ItemCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemCollision->SetSimulatePhysics(true);
		ItemCollision->SetGenerateOverlapEvents(true);
	}
	if (ItemMesh)
	{
		if (bNeedItemCollision)
		{
			ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else
		{
			ItemMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetGenerateOverlapEvents(false);
	}
	if (TriggerSphere)
	{
		TriggerSphere->SetCollisionProfileName(TEXT("IX_TriggerShape"));
		TriggerSphere->SetSimulatePhysics(false);
		TriggerSphere->CanCharacterStepUpOn = ECB_No;
		TriggerSphere->SetGenerateOverlapEvents(true);
		TriggerSphere->SetSphereRadius(280.0f);
	}
	if (ItemWidgetComponent)
	{
		ItemWidgetComponent->SetVisibility(false);
		ItemWidgetComponent->Deactivate();
	}
}

void ANAItemActor::PostInitProperties()
{
	Super::PostInitProperties();
}

void ANAItemActor::PostLoad()
{
	Super::PostLoad();
	
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (ItemDataID.IsNone() && !GetWorld()->IsPreviewWorld()
			&& !IsChildActor())
		{
			InitItemData();
		}
	}
}

void ANAItemActor::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();
}

void ANAItemActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	
	if (bNeedItemCollision && GetWorld()->IsGameWorld())
	{
		ReplaceRootWithItemCollisionIfNeeded();
	}
}

void ANAItemActor::PostActorCreated()
{
	Super::PostActorCreated();
}

void ANAItemActor::InitCheckIfChildActor()
{
	if (HasAuthority())
	{
		bWasChildActor = IsChildActor();
	}

	// ChildActorComponent에 의해 생성된 경우
	if (bWasChildActor || GetAttachParentActor() ||
		(RootComponent && RootComponent->GetAttachParent()
			&& RootComponent->GetAttachParent()->IsA<UChildActorComponent>()))
	{
		if (ItemCollision)
		{
			ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemCollision->SetCollisionProfileName(TEXT("NoCollision"));
			ItemCollision->SetSimulatePhysics(false);
			ItemCollision->SetGenerateOverlapEvents( false );
		}
		if (ItemMesh)
		{
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetGenerateOverlapEvents(false);
			ItemMesh->Deactivate();
		}
		if (TriggerSphere)
		{
			TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TriggerSphere->SetCollisionProfileName(TEXT("NoCollision"));
			TriggerSphere->SetSimulatePhysics(false);
			TriggerSphere->SetGenerateOverlapEvents(false);
			TriggerSphere->Deactivate();
			TriggerSphere->SetSphereRadius(0.0f);
		}
	}
}

EItemSubobjDirtyFlags ANAItemActor::GetDirtySubobjectFlags(
	const FNAItemBaseTableRow* MetaData) const
{
	EItemSubobjDirtyFlags DirtyFlags = EItemSubobjDirtyFlags::ISDF_None;
	if (!MetaData) { ensureAlways(false); return DirtyFlags; }
	if (bNeedItemCollision
		&& MetaData->CollisionShape != EItemCollisionShape::ICS_None)
	{
		bool bDirtyShape = false;
		bool bDirtyShapeProps = false;
		bDirtyShape |= ItemCollision == nullptr;
		bDirtyShapeProps |= ItemCollision == nullptr;
		if (ItemCollision) {
			const FCollisionShape Shape = ItemCollision->GetCollisionShape();
			switch (MetaData->CollisionShape)
			{
			case EItemCollisionShape::ICS_Sphere:
				bDirtyShape |= !Shape.IsSphere();
				bDirtyShapeProps |=
					Shape.GetSphereRadius()!= MetaData->CollisionSphereRadius;
				break;
			case EItemCollisionShape::ICS_Box:
				bDirtyShape |= !Shape.IsBox();
				bDirtyShapeProps |=
					Shape.GetExtent() != MetaData->CollisionBoxExtent;
				break;
			case EItemCollisionShape::ICS_Capsule:
				bDirtyShape |= !Shape.IsCapsule();
				bDirtyShapeProps |=
					Shape.GetCapsuleRadius() != MetaData->CollisionCapsuleSize.X;
				bDirtyShapeProps |=
					Shape.GetCapsuleHalfHeight() != MetaData->CollisionCapsuleSize.Y;
				break;
			default:
				break;
			}
			bDirtyShapeProps |= ItemCollision->GetRelativeScale3D() != MetaData->CollisionScale3D;
		}
		if (bDirtyShape)
		{
			EnumAddFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape);
		}
		if (bDirtyShapeProps)
		{
			EnumAddFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionProperties);
		}
	}
	
	if (MetaData->MeshType != EItemMeshType::IMT_None && bNeedItemMesh)
	{
		bool bDirtyMesh = false;
		bool bDirtyMeshProps = false;
		bDirtyMesh |= ItemMesh == nullptr;
		bDirtyMeshProps |= ItemMesh == nullptr;
		if (ItemMesh) {
			switch (MetaData->MeshType)
			{
			case EItemMeshType::IMT_Static:
				{
					UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ItemMesh);
					bDirtyMesh |= StaticMeshComp == nullptr;
					if (StaticMeshComp) {
						bDirtyMeshProps |=
							StaticMeshComp->GetStaticMesh() != MetaData->StaticMeshAssetData.StaticMesh;
						bDirtyMeshProps |=
							ItemFractureCollection != MetaData->StaticMeshAssetData.FractureCollection;
						bDirtyMeshProps |=
							ItemFractureCache != MetaData->StaticMeshAssetData.FractureCache;
					}
					break;
				}
			case EItemMeshType::IMT_Skeletal:
				{
					USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(ItemMesh);
					bDirtyMesh |= SkeletalMeshComp == nullptr;
					if (SkeletalMeshComp) {
						bDirtyMeshProps |=
							SkeletalMeshComp->GetSkeletalMeshAsset() != MetaData->SkeletalMeshAssetData.SkeletalMesh;
						
						bDirtyMeshProps |=
							SkeletalMeshComp->GetAnimClass() != MetaData->SkeletalMeshAssetData.AnimClass;
					}
					break;
				}
			default:
				break;
			}
			bDirtyMeshProps |= !ItemMesh->GetRelativeTransform().Equals(MetaData->MeshTransform);
		}
		if (bDirtyMesh)
		{
			EnumAddFlags( DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshType );
		}
		if (bDirtyMeshProps)
		{
			EnumAddFlags( DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshProperties );
		}
	}
	return DirtyFlags;
}

void ANAItemActor::ReplaceRootWithItemCollisionIfNeeded()
{
	if (!GetWorld()->IsGameWorld()) return;
	
	if (!bNeedItemCollision || !ItemCollision) return;
	USceneComponent* PreviousRootComponent = GetRootComponent();
	
	if (PreviousRootComponent == ItemCollision) return;
	FTransform PreviousTransform = PreviousRootComponent->GetComponentTransform();
	
	TArray<USceneComponent*> PreviousChildren;
	PreviousRootComponent->GetChildrenComponents(false, PreviousChildren);
	for ( auto It = PreviousChildren.CreateConstIterator(); It; ++It )
	{
		if ( USceneComponent* Attachable = Cast<USceneComponent>( *It ))
		{
			Attachable->DetachFromComponent( FDetachmentTransformRules::KeepRelativeTransform );
		}
	}
	PreviousRootComponent->ClearFlags(RF_Standalone | RF_Public);
	RemoveInstanceComponent(PreviousRootComponent);
	PreviousRootComponent->DestroyComponent();
	StubRootComponent = nullptr;
	SetRootComponent(ItemCollision);
	
	if (PreviousChildren.Num() > 0)
	{
		for (USceneComponent* Child : PreviousChildren)
		{
			if (Child == ItemCollision) continue;
			Child->AttachToComponent(ItemCollision, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	
	ItemCollision->SetWorldTransform(PreviousTransform);
}

#if WITH_EDITOR
void ANAItemActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateItemMetaData();
	SetItemSubobjectsPropsFromMetaData();
}

void ANAItemActor::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	UpdateItemMetaData();
	SetItemSubobjectsPropsFromMetaData();
}

void ANAItemActor::UpdateItemMetaData()
{
	if (!UNAItemEngineSubsystem::Get()
		|| !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
		|| !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass()))
		return;
	
	FNAItemBaseTableRow* MetaData
			= UNAItemEngineSubsystem::Get()->GetItemMetaDataStructs(GetClass());
	if (!MetaData) return;

	const ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(GetClass()->GetDefaultObject(false));
	if (!ItemActorCDO) return;

	if (ItemActorCDO->ItemCollision)
	{
		ItemActorCDO->ItemCollision->SetRelativeLocation(FVector::ZeroVector);
		ItemActorCDO->ItemCollision->SetRelativeRotation(FRotator::ZeroRotator);
	}
	
	const EItemSubobjDirtyFlags CDODirtyFlags = ItemActorCDO->GetDirtySubobjectFlags(MetaData);
	if (!EnumHasAnyFlags(CDODirtyFlags
		, EItemSubobjDirtyFlags::ISDF_CollisionProperties | EItemSubobjDirtyFlags::ISDF_MeshProperties)) return;
	
	if (EnumHasAnyFlags(CDODirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionProperties))
	{
		if (ItemActorCDO->ItemCollision)
		{
			if (USphereComponent* CDOSphereCollision = Cast<USphereComponent>(ItemActorCDO->ItemCollision))
			{
				if (MetaData->CollisionSphereRadius != CDOSphereCollision->GetScaledSphereRadius())
				{
					MetaData->CollisionSphereRadius = CDOSphereCollision->GetScaledSphereRadius();
				}
			}
			else if (UBoxComponent* CDOBoxCollision = Cast<UBoxComponent>(ItemActorCDO->ItemCollision))
			{
				if (MetaData->CollisionBoxExtent != CDOBoxCollision->GetScaledBoxExtent())
				{
					MetaData->CollisionBoxExtent = CDOBoxCollision->GetScaledBoxExtent();
				}
			}
			else if (UCapsuleComponent* CDOCapsuleCollision = Cast<UCapsuleComponent>(ItemActorCDO->ItemCollision))
			{
				if (MetaData->CollisionCapsuleSize.X != CDOCapsuleCollision->GetScaledCapsuleRadius())
				{
					MetaData->CollisionCapsuleSize.X = CDOCapsuleCollision->GetScaledCapsuleRadius();
				}
				if (MetaData->CollisionCapsuleSize.Y != CDOCapsuleCollision->GetScaledCapsuleHalfHeight())
				{
					MetaData->CollisionCapsuleSize.Y = CDOCapsuleCollision->GetScaledCapsuleHalfHeight();
				}
			}
			if (!MetaData->CollisionScale3D.Equals(ItemActorCDO->ItemCollision->GetRelativeScale3D()))
			{
				MetaData->CollisionScale3D = ItemActorCDO->ItemCollision->GetRelativeScale3D();
			}
		}
	}

	if (EnumHasAnyFlags(CDODirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshProperties))
	{
		if (ItemMesh)
		{
			if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ItemMesh))
			{
				if (MetaData->StaticMeshAssetData.StaticMesh != StaticMeshComp->GetStaticMesh())
				{
					MetaData->StaticMeshAssetData.StaticMesh = StaticMeshComp->GetStaticMesh();
				}
				if (MetaData->StaticMeshAssetData.FractureCollection != ItemFractureCollection)
				{
					MetaData->StaticMeshAssetData.FractureCollection = ItemFractureCollection;
				}
				if (MetaData->StaticMeshAssetData.FractureCache != ItemFractureCache)
				{
					MetaData->StaticMeshAssetData.FractureCache = ItemFractureCache;
				}
			}
			else if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(ItemMesh))
			{
				if (MetaData->SkeletalMeshAssetData.SkeletalMesh != SkeletalMeshComp->GetSkeletalMeshAsset())
				{
					MetaData->SkeletalMeshAssetData.SkeletalMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
				}
				if (MetaData->SkeletalMeshAssetData.AnimClass != SkeletalMeshComp->GetAnimClass())
				{
					MetaData->SkeletalMeshAssetData.AnimClass = SkeletalMeshComp->GetAnimClass();
				}
			}
			
			if (!MetaData->MeshTransform.Equals(ItemMesh->GetRelativeTransform()))
			{
				MetaData->MeshTransform = ItemMesh->GetRelativeTransform();
			}
		}
	}
}

void ANAItemActor::SetItemSubobjectsPropsFromMetaData()
{
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	if (!UNAItemEngineSubsystem::Get()
		|| !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
		|| !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass()))
		return;
	
	FNAItemBaseTableRow* MetaData
			= UNAItemEngineSubsystem::Get()->GetItemMetaDataStructs(GetClass());
	if (!MetaData) return;

	const ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(GetClass()->GetDefaultObject(false));
	if (!ItemActorCDO) return;
	
	
	if (ItemActorCDO->ItemCollision && ItemCollision)
	{
		// ItemCollision의 프로퍼티 복사
		// 먼저 메모리 스트림을 생성하고
		TArray<uint8> SavedBytes;
		FObjectWriter Writer(ItemActorCDO->ItemCollision, SavedBytes); // CDO의 ItemCollision 내용을 SavedBytes에 쓴다.
       
		// 저장된 바이트를 다시 읽어서 현재 객체의 ItemCollision에 적용한다.
		FObjectReader Reader(ItemCollision, SavedBytes); // SavedBytes 내용을 현재 ItemCollision에 읽어들인다.
	}
	if (ItemActorCDO->ItemMesh && ItemMesh)
	{
		TArray<uint8> SavedBytes;
		FObjectWriter Writer(ItemActorCDO->ItemMesh, SavedBytes);
		FObjectReader Reader(ItemMesh, SavedBytes);
	}
}

void ANAItemActor::PostCDOCompiled(const FPostCDOCompiledContext& Context)
{
	Super::PostCDOCompiled(Context);
	
	ReconstructItemSubobjectsFromMetaData();
}
#endif

void ANAItemActor::ReconstructItemSubobjectsFromMetaData()
{
	if (HasActorBegunPlay())
	{
		UE_LOG(NAItem, Warning, TEXT("[%hs] BeginPlay 이후에 호출되면 안됨."), __FUNCTION__);
		return;
	}
	
	if (!UNAItemEngineSubsystem::Get()
	|| !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
#if WITH_EDITOR
	|| !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass())
#endif
	) return;
	
	const FNAItemBaseTableRow* MetaData
		= UNAItemEngineSubsystem::Get()->GetItemMetaDataByClass(GetClass());
	if (!MetaData) return;

	const EItemSubobjDirtyFlags DirtyFlags = GetDirtySubobjectFlags(MetaData);
	if (EnumHasAnyFlags(DirtyFlags,
		EItemSubobjDirtyFlags::ISDF_CollisionShape | EItemSubobjDirtyFlags::ISDF_MeshType))
	{
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{	
			if (ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(GetClass()->GetDefaultObject(false)))
			{
				// CDO에서 서브오브젝트 계층 구조 수정 전이면 리턴.
				const EItemSubobjDirtyFlags CDODirtyFlags = ItemActorCDO->GetDirtySubobjectFlags(MetaData);
				if (EnumHasAnyFlags(CDODirtyFlags,
					EItemSubobjDirtyFlags::ISDF_CollisionShape | EItemSubobjDirtyFlags::ISDF_MeshType))
				{
					return;
				}
			}
		}

		UClass* NewItemCollisionClass = nullptr;
		if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape))
		{
			switch (MetaData->CollisionShape)
			{
			case EItemCollisionShape::ICS_Sphere:
				NewItemCollisionClass = USphereComponent::StaticClass();
				break;
			case EItemCollisionShape::ICS_Box:
				NewItemCollisionClass = UBoxComponent::StaticClass();
				break;
			case EItemCollisionShape::ICS_Capsule:
				NewItemCollisionClass = UCapsuleComponent::StaticClass();
				break;
			default:
				break;
			}
		
			if (NewItemCollisionClass && ItemCollision
				&& ItemCollision->GetClass() != NewItemCollisionClass)
			{
				ItemCollision->ClearFlags(RF_Standalone | RF_Public);
				ItemCollision->DestroyComponent();
				RemoveInstanceComponent(ItemCollision);
			}
		}

		UClass* NewItemMeshClass = nullptr;
		if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshType))
		{
			switch (MetaData->MeshType)
			{
			case EItemMeshType::IMT_Skeletal:
				NewItemMeshClass = USkeletalMeshComponent::StaticClass();
				break;
			case EItemMeshType::IMT_Static:
				NewItemMeshClass = UStaticMeshComponent::StaticClass();
				break;
			default:
				break;
			}
			if (NewItemMeshClass && ItemMesh
				&& ItemMesh->GetClass() != NewItemMeshClass)
			{
				ItemMesh->ClearFlags(RF_Standalone | RF_Public);
				ItemMesh->DestroyComponent();
				RemoveInstanceComponent(ItemMesh);
			}
		}

		// 에디터 런타임 중 바뀐 ItemCollision과 ItemMesh: 기본 생성자에 의해 객체는 만들어졌으나,
		// (현 시점에서) 프로퍼티에 담기지는 않음. 여기서 수동으로 재할당 + Attach Children 이전 
		for (UActorComponent* OwnedActorComp : GetComponents().Array())
		{
			if (!IsValid(OwnedActorComp)) continue;

			if (NewItemCollisionClass 
				&& OwnedActorComp->GetClass()->IsChildOf(NewItemCollisionClass)
				&& OwnedActorComp->GetName().StartsWith(TEXT("ItemCollision")))
			{
				if (UShapeComponent* NewItemCollision = Cast<UShapeComponent>(OwnedActorComp))
				{
					ItemCollision = NewItemCollision;
				}
			}
			else if (NewItemMeshClass
				&& OwnedActorComp->GetClass()->IsChildOf(NewItemMeshClass)
				&& OwnedActorComp->GetName().StartsWith(TEXT("ItemMesh")))
			{
				if (UMeshComponent* NewItemMesh = Cast<UMeshComponent>(OwnedActorComp))
				{
					ItemMesh = NewItemMesh;
				}
			}
		}
		
#if WITH_EDITOR
		if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(GetClass()))
		{
			TArray<USCS_Node*> SCSNodes =  BPGC->SimpleConstructionScript->GetAllNodes();
			if (SCSNodes.Num() > 0)
			{
				for (USCS_Node* SCSNode : SCSNodes)
				{
					FName ParentComponentName = SCSNode->ParentComponentOrVariableName;
				
					if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape))
					{
						if (ParentComponentName.ToString().StartsWith(TEXT("ItemCollision"))
							&& ParentComponentName != ItemCollision->GetFName())
						{
							SCSNode->ParentComponentOrVariableName = ItemCollision->GetFName();
						}
					}
					if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshType))
					{
						if (ParentComponentName.ToString().StartsWith(TEXT("ItemMesh"))
							&& ParentComponentName != ItemMesh->GetFName())
						{
							SCSNode->ParentComponentOrVariableName = ItemMesh->GetFName();
						}
					}
				}
			}
		}

		if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(GetClass())))
		{
			if (!BP->IsPossiblyDirty())
			{
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
				FKismetEditorUtilities::CompileBlueprint(
					BP,
					EBlueprintCompileOptions::SkipSave
					| EBlueprintCompileOptions::SkipGarbageCollection
					| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing
				);
			}
		}
#endif
	}
	
	// 부모, 자식에서 Property로 설정된 컴포넌트들을 조회
	// 최종적으로 프로퍼티에 남은 컴포넌트 주소들을 확인
	TSet<UActorComponent*> ItemActorSubobjects;
	for (TFieldIterator<FObjectProperty> It (GetClass()); It; ++It)
	{
		if (It->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
		{
			if (UActorComponent* Component = Cast<UActorComponent>(It->GetObjectPropertyValue_InContainer(this)))
			{
				ItemActorSubobjects.Add(Component);
			}
		}
	}
	if (ItemActorSubobjects.Num() != GetComponents().Num())
	{
		for (UActorComponent* OwnedActorComp : GetComponents().Array())
		{
			if (!IsValid(OwnedActorComp)) continue;
			if (USceneComponent* OwnedSceneComp = Cast<USceneComponent>(OwnedActorComp))
			{
				if (!ItemActorSubobjects.Contains(OwnedActorComp))
				{
					OwnedSceneComp->ClearFlags(RF_Standalone | RF_Public);
					OwnedSceneComp->DestroyComponent();
					RemoveInstanceComponent(OwnedSceneComp);
				}
			}
		}
	}
}

void ANAItemActor::OnConstruction(const FTransform& Transform)
{
 	Super::OnConstruction(Transform);
	
	ReconstructItemSubobjectsFromMetaData();
	
    // CDO 또는 Child Actor인 경우: 새로운 아이템 데이터 인스턴스 생성 안함
    if (!HasAnyFlags(RF_ClassDefaultObject) && ItemDataID.IsNone()
	    && !GetWorld()->IsPreviewWorld() && !IsChildActor())
    {
	    InitItemData();
    }
}

void ANAItemActor::Destroyed()
{
	if (HasActorBegunPlay() && IsPendingKillPending()
		&& ItemWidgetComponent && ItemWidgetComponent->IsVisible())
	{
		TransferItemWidgetToPopupBeforeDestroy();
	}
	Super::Destroyed();
}

void ANAItemActor::TransferItemWidgetToPopupBeforeDestroy() const
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANAItemWidgetPopupActor* Popup = GetWorld()->SpawnActor<ANAItemWidgetPopupActor>(
		ANAItemWidgetPopupActor::StaticClass(),
		GetRootComponent()->GetComponentTransform(),
		Params);

	ensureAlways(Popup);

	ItemWidgetComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Popup->InitializePopup(ItemWidgetComponent);
}

void ANAItemActor::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	DOREPLIFETIME( ANAItemActor, bWasChildActor );
}

void ANAItemActor::InitItemData()
{
	if (HasValidItemID()) return;
	if (const UNAItemData* NewItemData = UNAItemEngineSubsystem::Get()->CreateItemDataByActor(this))
	{
		ItemDataID = NewItemData->GetItemID();
		VerifyInteractableData();
	}
}

void ANAItemActor::VerifyInteractableData()
{
	if (InteractableInterfaceRef != nullptr) return;
	
	// 이 액터가 UNAInteractableInterface 인터페이스를 구현했다면 this를 할당
	if (HasValidItemID() && GetClass()->ImplementsInterface(UNAInteractableInterface::StaticClass()))
	{
		InteractableInterfaceRef = this;
	}
	else
	{
		ensureAlways(false);
	}
}

void ANAItemActor::FinalizeAndDestroyAfterInventoryAdded(AActor* Interactor)
{
	FinalizeAndDestroyAfterInventoryAdded_Impl(Interactor);
	Destroy();
}

void ANAItemActor::ReleaseItemWidgetComponent()
{
	if (ItemWidgetComponent && !ItemWidgetComponent->IsVisible())
	{
		ItemWidgetComponent->ReleaseItemWidgetPopup();
	}
}

void ANAItemActor::CollapseItemWidgetComponent()
{
	if (ItemWidgetComponent && ItemWidgetComponent->IsVisible())
	{
		ItemWidgetComponent->CollapseItemWidgetPopup();
	}
}

void ANAItemActor::OnActorBeginOverlap_Impl(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Execute_NotifyInteractableFocusBegin(this, OverlappedComponent->GetOwner(), OtherActor);
}

void ANAItemActor::OnActorEndOverlap_Impl(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Execute_NotifyInteractableFocusEnd(this,  OverlappedComponent->GetOwner(), OtherActor);
}

void ANAItemActor::BeginPlay()
{
	Super::BeginPlay();

	InitCheckIfChildActor();
	
	if (InteractableInterfaceRef && TriggerSphere)
	{
		TriggerSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnActorBeginOverlap_Impl);
		TriggerSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnActorEndOverlap_Impl);

		// 다음 틱에서 수동으로 오버랩 델리게이트를 직접 브로드캐스트
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::BroadcastInitialOverlapsOnTriggerSphere);
	}
	
	if (HasValidItemID())
	{
		// 임시: 수량 랜덤
		if (GetItemData()->IsStackableItem())
		{
			int32 RandomNumber = FMath::RandRange(1, GetItemData()->GetItemMaxSlotStackSize());
			GetItemData()->SetQuantity(RandomNumber);
		}
		else
		{
			GetItemData()->SetQuantity(1);
		}
	}
}

void ANAItemActor::BroadcastInitialOverlapsOnTriggerSphere()
{
	if (!TriggerSphere ||
		!TriggerSphere->GetGenerateOverlapEvents()) return;
	
	// 이미 겹친 액터들 가져와서 일괄 처리
	TArray<AActor*> Overlaps;
	TriggerSphere->GetOverlappingActors(Overlaps, ANACharacter::StaticClass());
	for (AActor* Other : Overlaps)
	{
		TriggerSphere->OnComponentBeginOverlap.Broadcast(
			TriggerSphere,
			Other,
			Cast<UPrimitiveComponent>(Other->GetRootComponent()),
			0,
			false,
			FHitResult{}
		);
	}
}

void ANAItemActor::PostNetReceive()
{
	Super::PostNetReceive();
}

void ANAItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UNAItemData* ANAItemActor::GetItemData() const
{
	return UNAItemEngineSubsystem::Get()->GetRuntimeItemData(ItemDataID);
}

bool ANAItemActor::HasValidItemID() const
{
	return !ItemDataID.IsNone();
}

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================

bool ANAItemActor::CanInteract_Implementation() const
{
	return IsValid(TriggerSphere)
			&& InteractableInterfaceRef != nullptr && bIsFocused;
}

bool ANAItemActor::IsOnInteract_Implementation() const
{
	return bIsOnInteract;
}

void ANAItemActor::NotifyInteractableFocusBegin_Implementation(AActor* InteractableActor, AActor* InteractorActor)
{
	if (UNAInteractionComponent* InteractionComp = GetInteractionComponent(InteractorActor))
	{
		if (const APawn* MaybePawn = Cast<APawn>(InteractorActor) )
		{
			FString ItemName = InteractorActor ? GetNameSafe(InteractableActor) : TEXT_NULL;
			FString InteractorName = InteractorActor ? GetNameSafe(InteractorActor) : TEXT_NULL;
			UE_LOG(NAInteraction, Log, TEXT("[NotifyInteractableFocusBegin] 포커스 On. 아이템: %s, 행위자: %s")
				   , *ItemName, *InteractorName);
			bIsFocused = InteractionComp->OnInteractableFound(this);

			if ( MaybePawn->IsLocallyControlled() )
			{
				if (bIsFocused && IsValid(ItemWidgetComponent))
				{
					ReleaseItemWidgetComponent();
				}
			}
		}
	}
}

void ANAItemActor::NotifyInteractableFocusEnd_Implementation(AActor* InteractableActor, AActor* InteractorActor)
{
	if (UNAInteractionComponent* InteractionComp = GetInteractionComponent(InteractorActor))
	{
		if (const APawn* MaybePawn = Cast<APawn>(InteractorActor) )
		{
			FString ItemName = InteractorActor ? GetNameSafe(InteractableActor) : TEXT_NULL;
			FString InteractorName = InteractorActor ? GetNameSafe(InteractorActor) : TEXT_NULL;
			UE_LOG(NAInteraction, Log, TEXT("[NotifyInteractableFocusEnd] 포커스 Off. 아이템: %s, 행위자: %s")
				   , *ItemName, *InteractorName);
			bIsFocused = !InteractionComp->OnInteractableLost(this);

			if ( MaybePawn->IsLocallyControlled() )
			{
				if (!bIsFocused && IsValid(ItemWidgetComponent))
				{
					CollapseItemWidgetComponent();
				}
			}
		}
	}
}

bool ANAItemActor::TryInteract_Implementation(AActor* Interactor)
{
	bIsOnInteract = true;
	SetInteractionPhysicsEnabled(false);
	
	if (Execute_BeginInteract(this, Interactor))
	{
		if (Execute_ExecuteInteract(this, Interactor))
		{
			if (Execute_EndInteract(this, Interactor))
			{
				if (!IsUnlimitedInteractable())
				{
					SetInteractableCount(GetInteractableCount() - 1);
				}
				
				UE_LOG(NAInteraction, Log, TEXT("[TryInteract] 상호작용 완료"));
				SetInteractionPhysicsEnabled(true);
				bIsOnInteract = false;
				return true;
			}
		}
	}

	UE_LOG(NAInteraction, Log, TEXT("[TryInteract] 상호작용 중단"));
	SetInteractionPhysicsEnabled(true);
	bIsOnInteract = false;
	return false;
}

bool ANAItemActor::BeginInteract_Implementation(AActor* InteractorActor)
{
	if (!Execute_CanInteract(this)) { return false; }
	if (!CanPerformInteractionWith(InteractorActor))
	{
		UE_LOG(NAInteraction, Warning, TEXT("[BeginInteract] 상호작용 조건 불충분"));
		return false;
	}
	return bIsOnInteract;
}

bool ANAItemActor::ExecuteInteract_Implementation(AActor* InteractorActor)
{
	ensureAlwaysMsgf(bIsOnInteract, TEXT("[ExecuteInteract] bIsOnInteract이 false였음"));
	return bIsOnInteract;
}

bool ANAItemActor::EndInteract_Implementation(AActor* InteractorActor)
{
	return bIsOnInteract;
}

bool ANAItemActor::TryGetInteractableData(FNAInteractableData& OutData) const
{
	if (UNAItemData* ItemData = GetItemData())
	{
		return ItemData->GetInteractableData(OutData);
	}
	return false;
}

bool ANAItemActor::HasInteractionDelay() const
{
	FNAInteractableData Data;
	if (GetItemData() && GetItemData()->GetInteractableData(Data))
	{
		return Data.InteractionDelayTime > 0.f;
	}
	return false;
}

float ANAItemActor::GetInteractionDelay() const
{
	FNAInteractableData Data;
	if (GetItemData() && GetItemData()->GetInteractableData(Data))
	{
		return Data.InteractionDelayTime;
	}
	return 0.f;
}

bool ANAItemActor::IsAttachedAndPendingUse() const
{
	return bIsAttachedAndPendingUse && IsChildActor();
}

void ANAItemActor::SetAttachedAndPendingUse(bool bNewState)
{
	if (bNewState && !IsChildActor())
	{
		ensureAlways(false);
		return;
	}
	
	bIsAttachedAndPendingUse = bNewState;
}

bool ANAItemActor::IsUnlimitedInteractable() const
{
	FNAInteractableData Data;
	if (TryGetInteractableData(Data))
	{
		return Data.bIsUnlimitedInteractable;
	}
	return false;
}

int32 ANAItemActor::GetInteractableCount() const
{
	FNAInteractableData Data;
	if (TryGetInteractableData(Data))
	{
		return Data.InteractableCount;
	}
	return -1;
}

void ANAItemActor::SetInteractableCount(int32 NewCount)
{
	FNAInteractableData Data;
	if (TryGetInteractableData(Data))
	{
		Data.InteractableCount = NewCount;
	}
}

bool ANAItemActor::CanPerformInteractionWith(AActor* Interactor) const
{
	bool bCanPerform = Interactor && GetInteractionComponent(Interactor);
	
	FNAInteractableData Data;
	if (TryGetInteractableData(Data))
	{
		bCanPerform = bCanPerform && Data.InteractableType != ENAInteractableType::None;
		if (!Data.bIsUnlimitedInteractable)
		{
			bCanPerform = bCanPerform && Data.InteractableCount > 0;
		}
		return bCanPerform;
	}
	return bCanPerform;
}

ANAItemWidgetPopupActor::ANAItemWidgetPopupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PopupSceneRoot")));
}

void ANAItemWidgetPopupActor::InitializePopup(UNAItemWidgetComponent* NewPopupWidgetComponent)
{
	if (HasActorBegunPlay() && GetRootComponent() && !PopupWidgetComponent
		&& NewPopupWidgetComponent && NewPopupWidgetComponent->GetItemWidget())
	{
		NewPopupWidgetComponent->Rename(nullptr, this,REN_DontCreateRedirectors | REN_DoNotDirty);
		PopupWidgetComponent = NewPopupWidgetComponent;
		AddInstanceComponent(PopupWidgetComponent);
		PopupWidgetComponent->AttachToComponent(GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
		if (!PopupWidgetComponent->HasBeenCreated())
		{
			PopupWidgetComponent->OnComponentCreated();
		}
		PopupWidgetComponent->RegisterComponent();

		PopupWidgetComponent->GetItemWidget()->OnItemWidgetCollapseFinishedForDestroy.BindUObject(this, &ThisClass::OnCollapseAnimationFinished);
		PopupWidgetComponent->CollapseItemWidgetPopup();
	}
}

void ANAItemWidgetPopupActor::OnCollapseAnimationFinished()
{
	Destroy();
}
