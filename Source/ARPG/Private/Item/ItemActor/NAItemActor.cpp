
#include "Item/ItemActor/NAItemActor.h"

#include "FileHelpers.h"
#include "NACharacter.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Interaction/NAInteractionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemWidget/NAItemWidget.h"
#include "Misc/NALogCategory.h"

#if WITH_EDITOR || WITH_EDITORONLY_DATA
#include "Kismet2/KismetEditorUtilities.h"
#endif

bool ANAItemActor::bShouldDoLazyCompile = false;

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
		}
	}
	
	TriggerSphere = CreateDefaultSubobject<USphereComponent>("TriggerSphere");

	ItemWidgetComponent
		= CreateOptionalDefaultSubobject<UNAItemWidgetComponent>(TEXT("ItemWidgetComponent"));
	
	InitItemSubobjectsAttachment();
	InitItemSubobjectsProperties();
	
	bAlwaysRelevant = true;
	bReplicates = true;
	SetReplicateMovement(true);
	
	ItemDataID = NAME_None;
}

void ANAItemActor::InitItemSubobjectsAttachment()
{
	if (!ensureAlways(GetRootComponent() != nullptr && !bInitItemSubobjectsAttachment)) return;
	
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

	bInitItemSubobjectsAttachment = true;
}

void ANAItemActor::InitItemSubobjectsProperties()
{
	if (!ensureAlways(!bInitItemSubobjectsProperties)) return;
	
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

	bInitItemSubobjectsProperties = true;
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

#if WITH_EDITOR || WITH_EDITORONLY_DATA
void ANAItemActor::UpdateItemMetaData()
{
	if (!UNAItemEngineSubsystem::Get()
		|| !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
		|| !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass()))
		return;
	
	FNAItemBaseTableRow* MetaData
			= UNAItemEngineSubsystem::Get()->GetItemMetaDataStructs(GetClass());
	if (!MetaData) return;

	if (ItemCollision)
	{
		if (USphereComponent* SphereCollision = Cast<USphereComponent>(ItemCollision))
		{
			MetaData->CollisionSphereRadius = SphereCollision->GetScaledSphereRadius();
		}
		else if (UBoxComponent* BoxCollision = Cast<UBoxComponent>(ItemCollision))
		{
			MetaData->CollisionBoxExtent = BoxCollision->GetScaledBoxExtent();
		}
		else if (UCapsuleComponent* CapsuleCollision = Cast<UCapsuleComponent>(ItemCollision))
		{
			MetaData->CollisionCapsuleSize.X = CapsuleCollision->GetScaledCapsuleRadius();
			MetaData->CollisionCapsuleSize.Y = CapsuleCollision->GetScaledCapsuleHalfHeight();
		}
	}

	if (ItemMesh)
	{
		if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ItemMesh))
		{
			MetaData->StaticMeshAssetData.StaticMesh = StaticMeshComp->GetStaticMesh();
			MetaData->StaticMeshAssetData.FractureCollection = ItemFractureCollection;
			MetaData->StaticMeshAssetData.FractureCache = ItemFractureCache;
		}
		else if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(ItemMesh))
		{
			MetaData->SkeletalMeshAssetData.SkeletalMesh = SkeletalMeshComp->GetSkeletalMeshAsset();
			MetaData->SkeletalMeshAssetData.AnimClass = SkeletalMeshComp->GetAnimClass();
		}
		MetaData->MeshTransform = ItemMesh->GetRelativeTransform();
	}
}

void ANAItemActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateItemMetaData();
}

void ANAItemActor::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	UpdateItemMetaData();
}

void ANAItemActor::PostCDOCompiled(const FPostCDOCompiledContext& Context)
{
	Super::PostCDOCompiled(Context);
	
	ReconstructItemSubobjectsFromMetaData();
}
#endif

void ANAItemActor::ReconstructItemSubobjectsFromMetaData()
{
	if (!UNAItemEngineSubsystem::Get()
	|| !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
#if WITH_EDITOR || WITH_EDITORONLY_DATA
	|| !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass())
#endif
	) return;
	
	const FNAItemBaseTableRow* MetaData
		= UNAItemEngineSubsystem::Get()->GetItemMetaDataByClass(GetClass());
	if (!MetaData) return;

	Modify();
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
		struct FNASubobjectAttachInfo
		{
			USceneComponent* Subobject;
			FName AttachSocketName;
		};

		UClass* NewItemCollisionClass = nullptr;
		TArray<FNASubobjectAttachInfo> OldItemCollisionChildren;
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
		
			if (NewItemCollisionClass && ItemCollision)
			{
				if (HasAnyFlags(RF_ClassDefaultObject))
				{
					for (UActorComponent* OwnedActorComp : GetComponents().Array())
					{
						if (OwnedActorComp == GetRootComponent()) continue;
						if (USceneComponent* OwnedSceneComp = Cast<USceneComponent>(OwnedActorComp))
						{
							if (OwnedSceneComp->GetAttachParent() == ItemCollision)
							{
								FNASubobjectAttachInfo AttachInfo;
								AttachInfo.Subobject = OwnedSceneComp;
								AttachInfo.AttachSocketName = OwnedSceneComp->GetAttachSocketName();
								OldItemCollisionChildren.Add(AttachInfo);
								OwnedSceneComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
							}
						}
					}
				}
				else
				{
					if (ItemCollision->GetAttachChildren().Num() > 0)
					{
						TArray<USceneComponent*> OldAttachChildren = ItemCollision->GetAttachChildren();
						for (USceneComponent* AttachChild : OldAttachChildren)
						{
							if (IsValid(AttachChild))
							{
								FNASubobjectAttachInfo AttachInfo;
								AttachInfo.Subobject = AttachChild;
								AttachInfo.AttachSocketName = AttachChild->GetAttachSocketName();
								OldItemCollisionChildren.Add(AttachInfo);
								AttachChild->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
							}
						}
					}
				}

				if (ItemCollision->GetClass() != NewItemCollisionClass)
				{
					ItemCollision->ClearFlags(RF_Standalone | RF_Public);
					ItemCollision->DestroyComponent();
					RemoveInstanceComponent(ItemCollision);
				}
			}
		}

		UClass* NewItemMeshClass = nullptr;
		TArray<FNASubobjectAttachInfo> OldItemMeshChildren;
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
			if (NewItemMeshClass && ItemMesh)
			{
				if (HasAnyFlags(RF_ClassDefaultObject))
				{
					for (UActorComponent* OwnedActorComp : GetComponents().Array())
					{
						if (OwnedActorComp == GetRootComponent()) continue;
						if (USceneComponent* OwnedSceneComp = Cast<USceneComponent>(OwnedActorComp))
						{
							if (OwnedSceneComp->GetAttachParent() == ItemMesh)
							{
								FNASubobjectAttachInfo AttachInfo;
								AttachInfo.Subobject = OwnedSceneComp;
								AttachInfo.AttachSocketName = OwnedSceneComp->GetAttachSocketName();
								OldItemMeshChildren.Add(AttachInfo);
								OwnedSceneComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
							}
						}
					}
				}
				else
				{
					if (ItemMesh->GetAttachChildren().Num() > 0)
					{
						TArray<USceneComponent*> OldAttachChildren = ItemMesh->GetAttachChildren();
						for (USceneComponent* AttachChild : OldAttachChildren)
						{
							if (IsValid(AttachChild))
							{
								FNASubobjectAttachInfo AttachInfo;
								AttachInfo.Subobject = AttachChild;
								AttachInfo.AttachSocketName = AttachChild->GetAttachSocketName();
								OldItemMeshChildren.Add(AttachInfo);
								AttachChild->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
							}
						}
					}
				}
			
				if (ItemMesh->GetClass() != NewItemMeshClass)
				{
					ItemMesh->ClearFlags(RF_Standalone | RF_Public);
					ItemMesh->DestroyComponent();
					RemoveInstanceComponent(ItemMesh);
				}
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
					if (OldItemCollisionChildren.Num() > 0)
					{
						for (FNASubobjectAttachInfo ItemCollisionChild : OldItemCollisionChildren)
						{
							if (IsValid(ItemCollisionChild.Subobject))
							{
								if (HasAnyFlags(RF_ClassDefaultObject))
								{
									ItemCollisionChild.Subobject->SetupAttachment(ItemCollision
										, ItemCollisionChild.AttachSocketName);
								}
								else
								{
									ItemCollisionChild.Subobject->AttachToComponent(ItemCollision
										, FAttachmentTransformRules::KeepRelativeTransform
										, ItemCollisionChild.AttachSocketName);
								}
							}
						}
					}
				}
			}
			else if (NewItemMeshClass
				&& OwnedActorComp->GetClass()->IsChildOf(NewItemMeshClass)
				&& OwnedActorComp->GetName().StartsWith(TEXT("ItemMesh")))
			{
				if (UMeshComponent* NewItemMesh = Cast<UMeshComponent>(OwnedActorComp))
				{
					ItemMesh = NewItemMesh;
					if (OldItemMeshChildren.Num() > 0)
					{
						for (FNASubobjectAttachInfo ItemMeshChild : OldItemMeshChildren)
						{
							if (IsValid(ItemMeshChild.Subobject))
							{
								if (HasAnyFlags(RF_ClassDefaultObject))
								{
									ItemMeshChild.Subobject->SetupAttachment(ItemMesh
										, ItemMeshChild.AttachSocketName);
								}
								else
								{
									ItemMeshChild.Subobject->AttachToComponent(ItemMesh
										, FAttachmentTransformRules::KeepRelativeTransform
										, ItemMeshChild.AttachSocketName);
								}
							}
						}
					}
				}
			}
		}
		OldItemCollisionChildren.Empty();
		OldItemMeshChildren.Empty();

#if WITH_EDITOR || WITH_EDITORONLY_DATA
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
	//if (ItemActorSubobjects.Num() != GetComponents().Num())
	{
		for (UActorComponent* OwnedActorComp : GetComponents().Array())
		{
			if (!IsValid(OwnedActorComp)) continue;
			if (USceneComponent* OwnedSceneComp = Cast<USceneComponent>(OwnedActorComp))
			{
				if (ItemActorSubobjects.Contains(OwnedActorComp))
				{
					if (OwnedSceneComp != GetRootComponent()
						&& OwnedSceneComp->GetAttachChildren().Num() > 0)
					{
						TArray<USceneComponent*> SubobjChildren = OwnedSceneComp->GetAttachChildren();
						for (USceneComponent* Child : SubobjChildren)
						{
							if (!IsValid(Child)) continue;
					
							if (Child->GetAttachParent() != OwnedSceneComp)
							{
								Child->AttachToComponent(OwnedSceneComp
									, FAttachmentTransformRules::KeepRelativeTransform
									, Child->GetAttachSocketName());
							}
						}
					}
				}
				else
				{
					OwnedSceneComp->ClearFlags(RF_Standalone | RF_Public);
					OwnedSceneComp->DestroyComponent();
					RemoveInstanceComponent(OwnedSceneComp);
				}
			}
		}
	}
	MarkPackageDirty();
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

    if (!UNAItemEngineSubsystem::Get()
	    || !UNAItemEngineSubsystem::Get()->IsItemMetaDataInitialized()
#if WITH_EDITOR || WITH_EDITORONLY_DATA
	    || !UNAItemEngineSubsystem::Get()->IsRegisteredItemMetaClass(GetClass())
#endif
    )
	    return;

    const FNAItemBaseTableRow* MetaData
	    = UNAItemEngineSubsystem::Get()->GetItemMetaDataByClass(GetClass());
    if (!MetaData) return;

	const EItemSubobjDirtyFlags DirtyFlags = GetDirtySubobjectFlags(MetaData);
   
	if (MetaData->CollisionShape != EItemCollisionShape::ICS_None)
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
	
	}
	if (MetaData->MeshType != EItemMeshType::IMT_None)
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
	}
	
	if (GetRootComponent())
	{
		GetRootComponent()->SetWorldTransform(Transform);
	}
	if (ItemCollision)
	{
		if (GetRootComponent() != ItemCollision)
		{
			ItemCollision->SetRelativeTransform(FTransform::Identity);
		}
	}
	if (ItemMesh && !GetWorld()->IsPreviewWorld())
	{
		ItemMesh->SetRelativeTransform(MetaData->MeshTransform);
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

	{
		// if (HasAuthority())
		// {
		// 	// 서버에서 Owner가 없었다면 클라이언트 플레이어에게 소유권 할당
		// 	if (!GetOwner())
		// 	{
		// 		if (APawn* InstigatingPawn = GetInstigator())
		// 		{
		// 			if (AController* InstigatingController = InstigatingPawn->GetController())
		// 			{
		// 				SetOwner(InstigatingController);
		// 				UE_LOG(NAItem, Log, TEXT("[Server] %s: Set owner to Controller %s")
		// 					, *GetName(), *InstigatingController->GetName());
		// 			}else
		// 			{
		// 				UE_LOG(NAItem, Warning, TEXT("[Server] %s: Could not find controller from instigator %s"), *GetName(), *InstigatingPawn->GetName());
		// 			}
		// 		}
		// 		else
		// 		{
		// 			UE_LOG(NAItem, Warning, TEXT("[Server] %s: No instigator found to determine owner."), *GetName());
		// 		}
		// 	}
		// }
		// else
		// {
		// 	// 클라이언트: Owner 정보 로그 출력 (RPC가 가능한 상태인지 확인용)
		// 	if (AActor* MyOwner = GetOwner())
		// 	{
		// 		UE_LOG(NAItem, Log, TEXT("[Client] %s: Owner is %s"), *GetName(), *MyOwner->GetName());
		// 	}
		// 	else
		// 	{
		// 		UE_LOG(NAItem, Warning, TEXT("[Client] %s: No owner assigned! RPCs will fail."), *GetName());
		// 	}
		// }
	}
	
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
