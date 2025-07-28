
#include "Item/ItemActor/NAItemActor.h"

#include "NACharacter.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interaction/NAInteractionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemWidget/NAItemWidget.h"
#include "Misc/NALogCategory.h"

#if WITH_EDITOR
#include "Item/NAEditor/FNAEdItemBridgeService.h"
#include "NAEditor_Item/ItemActorEditor/NAEdItemActorEditorUtils.h"
#endif

class UBlueprintVariableNodeSpawner;

ANAItemActor::ANAItemActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	/*if (HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!GetClass()->HasAllClassFlags(CLASS_CompiledFromBlueprint))
		{
			UE_LOG(NAItem, Log, TEXT("[ANAItemActor] C++ CDO (%s)"), *GetNameSafe(this));
		}
		else
		{
			UE_LOG(NAItem, Log, TEXT("[ANAItemActor] BP CDO (%s)"), *GetNameSafe(this));
		}
	}
	else
	{
		if (!GetClass()->HasAllClassFlags(CLASS_CompiledFromBlueprint))
		{
			UE_LOG(NAItem, Log, TEXT("[ANAItemActor] C++ 인스턴스 (%s)"), *GetNameSafe(this));
		}
		else
		{
			UE_LOG(NAItem, Log, TEXT("[ANAItemActor] BP 인스턴스 (%s)"), *GetNameSafe(this));
		}
	}*/

	StubRootComponent = CreateDefaultSubobject<USceneComponent>("StubRootComponent");
	SetRootComponent( StubRootComponent );

	if (UNAItemEngineSubsystem* ItemEngineSubsystem = UNAItemEngineSubsystem::Get())
	{
		if (const FNAItemBaseTableRow* MetaData = ItemEngineSubsystem->FindItemMetaData(GetClass()))
		{
			switch (MetaData->CollisionShape)
			{
			case EItemCollisionShape::ICS_Sphere:
				ItemCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ItemCollision(Sphere)"));
				break;
			case EItemCollisionShape::ICS_Box:
				ItemCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ItemCollision(Box)"));
				break;
			case EItemCollisionShape::ICS_Capsule:
				ItemCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ItemCollision(Capsule)"));
				break;
			default:
				bNeedItemCollision = false;
				break;
			}
			if (ItemCollision)
			{
				ItemCollision->SetRelativeTransform(FTransform::Identity);
			}
			
			switch (MetaData->MeshType)
			{
			case EItemMeshType::IMT_Static:
				ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh(Static)"));
				break;
			case EItemMeshType::IMT_Skeletal:
				ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh(Skeletal)"));
				break;
			default:
				bNeedItemMesh = false;
				break;
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
		ItemCollision->SetSimulatePhysics(true);
		ItemCollision->SetGenerateOverlapEvents(true);
		ItemCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		ItemCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (ItemMesh)
	{
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetGenerateOverlapEvents(false);
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
		ItemMesh->CanCharacterStepUpOn = ECB_No;
	}
	if (TriggerSphere)
	{
		TriggerSphere->SetSimulatePhysics(false);
		TriggerSphere->SetGenerateOverlapEvents(true);
		TriggerSphere->SetCollisionProfileName(TEXT("IX_TriggerShape"));
		TriggerSphere->CanCharacterStepUpOn = ECB_No;
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

	if (!UNAItemEngineSubsystem::Get()) return;
#if WITH_EDITOR
	// 메타데이터 인스턴싱 도중 로드된 경우
	if (FNAEdItemBridge::GetItemRegistrationPhase(GetClass())
		== EItemEditorRegistrationPhase::DuringInstancing)
	{
		BackupItemSubobjectPropertiesToMetaData();
	}
#endif
	
	InitItemData();
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
			ItemCollision->SetSimulatePhysics(false);
			ItemCollision->SetGenerateOverlapEvents( false );
			ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemCollision->SetCollisionProfileName(TEXT("NoCollision"));
			ItemCollision->Deactivate();
		}
		if (ItemMesh)
		{
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetGenerateOverlapEvents(false);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetCollisionProfileName(TEXT("NoCollision"));
			ItemMesh->Deactivate();
		}
		if (TriggerSphere)
		{
			TriggerSphere->SetSimulatePhysics(false);
			TriggerSphere->SetGenerateOverlapEvents(false);
			TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TriggerSphere->SetCollisionProfileName(TEXT("NoCollision"));
			TriggerSphere->Deactivate();
			TriggerSphere->SetSphereRadius(0.0f);
		}
	}
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
EItemSubobjDirtyFlags ANAItemActor::GetCurrentDirtyFlags() const
{
	const FNAItemBaseTableRow* MetaData = UNAItemEngineSubsystem::Get()
		                                      ? UNAItemEngineSubsystem::Get()->FindItemMetaData(GetClass())
		                                      : nullptr;
	return ComputeDirtyFlagsFromMeta(MetaData);
}

EItemSubobjDirtyFlags ANAItemActor::ComputeDirtyFlagsFromMeta(const FNAItemBaseTableRow* MetaData) const
{
	if (!MetaData) return EItemSubobjDirtyFlags::ISDF_None;
	
	EItemSubobjDirtyFlags DirtyFlags = EItemSubobjDirtyFlags::ISDF_None;
	
	if (bNeedItemCollision && MetaData->CollisionShape != EItemCollisionShape::ICS_None)
	{
		bool bDirtyShape = false;
		bool bDirtyShapeProps = false;
		bDirtyShape |= ItemCollision == nullptr;
		bDirtyShapeProps |= ItemCollision == nullptr;
		if (ItemCollision)
		{
			const FCollisionShape Shape = ItemCollision->GetCollisionShape();
			switch (MetaData->CollisionShape)
			{
			case EItemCollisionShape::ICS_Sphere:
				bDirtyShape |= !Shape.IsSphere();
				bDirtyShapeProps |=
					Shape.GetSphereRadius() != MetaData->CollisionSphereRadius;
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
	
	if (bNeedItemMesh && MetaData->MeshType != EItemMeshType::IMT_None)
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
							!StaticMeshComp->GetRelativeTransform().Equals(MetaData->StaticMeshAssetData.StaticMeshTransform);
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
							!SkeletalMeshComp->GetRelativeTransform().Equals(MetaData->SkeletalMeshAssetData.SkeletalMeshTransform);
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

// Item Actor는 Row Struct에 브로드캐스트만 하고, 실질적인 데이터 백업은 Row Struct에서
void ANAItemActor::BackupItemSubobjectPropertiesToMetaData() const
{
	if (!HasAnyFlags(RF_ClassDefaultObject)) return;

	if (!FNAEdItemBridge::IsRegisteredItemMetaClass(GetClass())) return;
    
    FNAItemBaseTableRow* MetaData = reinterpret_cast<FNAItemBaseTableRow*>(FNAEdItemBridge::FindItemMetaDataForEditing(GetClass()));
	if (!ensureAlways(MetaData)) return;
	
    const EItemSubobjDirtyFlags CDODirtyFlags = ComputeDirtyFlagsFromMeta(MetaData);
	if (!EnumHasAnyFlags(CDODirtyFlags
		, EItemSubobjDirtyFlags::ISDF_CollisionProperties | EItemSubobjDirtyFlags::ISDF_MeshProperties)) return;
    
    if (EnumHasAnyFlags(CDODirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionProperties))
    {
        if (ItemCollision)
        {
            if (USphereComponent* CDOSphereCollision = Cast<USphereComponent>(ItemCollision))
            {
                if (MetaData->CollisionSphereRadius != CDOSphereCollision->GetScaledSphereRadius())
                {
                    MetaData->CollisionSphereRadius = CDOSphereCollision->GetScaledSphereRadius();
                }
            }
            else if (UBoxComponent* CDOBoxCollision = Cast<UBoxComponent>(ItemCollision))
            {
                if (MetaData->CollisionBoxExtent != CDOBoxCollision->GetScaledBoxExtent())
                {
                    MetaData->CollisionBoxExtent = CDOBoxCollision->GetScaledBoxExtent();
                }
            }
            else if (UCapsuleComponent* CDOCapsuleCollision = Cast<UCapsuleComponent>(ItemCollision))
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
            	if (!MetaData->StaticMeshAssetData.StaticMeshTransform.Equals(StaticMeshComp->GetRelativeTransform()))
            	{
            		MetaData->StaticMeshAssetData.StaticMeshTransform = StaticMeshComp->GetRelativeTransform();
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
            	if (!MetaData->SkeletalMeshAssetData.SkeletalMeshTransform.Equals(SkeletalMeshComp->GetRelativeTransform()))
            	{
            		MetaData->SkeletalMeshAssetData.SkeletalMeshTransform = SkeletalMeshComp->GetRelativeTransform();
            	}
            }
        }
    }
	
	FNAEdItemBridge::MarkMetaDataTableDirty(GetClass());
}

void ANAItemActor::PostCDOCompiled(const FPostCDOCompiledContext& Context)
{
	Super::PostCDOCompiled(Context);

	ReconstructItemSubobjectsFromMetaData();
}

void ANAItemActor::HandleItemClassRegisteredToMetaData(EItemEditorRegistrationPhase RegistrationPhase)
{
	check(FNAEdItemBridge::IsRegisteredItemMetaClass(GetClass()));
	check(HasAnyFlags(RF_ClassDefaultObject));

	EnsureForceNonDataOnlyVariableUsed();
	
	bool bShouldCompile = false;
	switch (RegistrationPhase)
	{
	case EItemEditorRegistrationPhase::DuringInstancing:
		// 블루프린트 CDO 동적 초기화 후 재컴파일 → 동적 초기화한 내용을 블프 에디터 패널에 반영하기 위함
		bShouldCompile = true;
		break;
	case EItemEditorRegistrationPhase::DuringEditorRuntime:
		// 에디터 런타임 중 메타데이터에 등록된 경우
		bShouldCompile = GetCurrentDirtyFlags() != EItemSubobjDirtyFlags::ISDF_None;
		break;
	default:
		check(false);
		break;
	}
	
	if (bShouldCompile)
	{
		if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(GetClass())))
		{
			FNAEdItemActorEditorUtils::CompileBlueprintWithOptionalStructuralMark(BP);
		}
		if (RegistrationPhase == EItemEditorRegistrationPhase::DuringEditorRuntime)
		{
			MarkPackageDirty();
		}
	}

	MarkPackageDirty();
}

void ANAItemActor::ReconstructItemSubobjectsFromMetaData()
{
	check(!GetWorld() || !GetWorld()->HasBegunPlay());
	if (!FNAEdItemBridge::IsRegisteredItemMetaClass(GetClass())) return;

	ReconstructItemSubobjectsFromMetaData_Impl();

	// ItemCollision는 런타임 때 루트 컴포넌트로 설정되므로, 그 전까지 트랜스폼을 항상 FTransform::Identity로 유지.
	if (bNeedItemCollision && ItemCollision)
	{
		ItemCollision->SetRelativeTransform(FTransform::Identity);
	}

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(GetClass())))
		{
			if (!BP->IsPossiblyDirty())
			{
				FNAEdItemActorEditorUtils::CompileBlueprintWithOptionalStructuralMark(
					BP, true);
			}
		}
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

void ANAItemActor::ReconstructItemSubobjectsFromMetaData_Impl()
{
	const FNAItemBaseTableRow* MetaData = UNAItemEngineSubsystem::Get()->FindItemMetaData(GetClass());
	check(MetaData);

	const EItemSubobjDirtyFlags DirtyFlags = ComputeDirtyFlagsFromMeta(MetaData);
	bool bShouldReconstruct = false;
	
	UClass* NewItemCollisionClass = nullptr;
	if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape))
	{
		bShouldReconstruct = true;
		
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
		bShouldReconstruct = true;
		
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

	if (bShouldReconstruct)
	{
		// 에디터 런타임 중 바뀐 ItemCollision과 ItemMesh: 기본 생성자에 의해 객체는 만들어졌으나,
		// (현 시점에서) 프로퍼티에 담기지는 않음. 여기서 수동으로 재할당
		for (UActorComponent* OwnedActorComp : GetComponents().Array())
		{
			if (!IsValid(OwnedActorComp)) continue;

			if (bNeedItemCollision && NewItemCollisionClass
				&& OwnedActorComp->GetClass()->IsChildOf(NewItemCollisionClass)
				&& OwnedActorComp->GetName().StartsWith(TEXT("ItemCollision")))
			{
				if (UShapeComponent* NewItemCollision = Cast<UShapeComponent>(OwnedActorComp))
				{
					ItemCollision = NewItemCollision;

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
			}
			if (bNeedItemMesh && NewItemMeshClass
				&& OwnedActorComp->GetClass()->IsChildOf(NewItemMeshClass)
				&& OwnedActorComp->GetName().StartsWith(TEXT("ItemMesh")))
			{
				if (UMeshComponent* NewItemMesh = Cast<UMeshComponent>(OwnedActorComp))
				{
					ItemMesh = NewItemMesh;

					if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ItemMesh))
					{
						StaticMeshComp->SetStaticMesh(MetaData->StaticMeshAssetData.StaticMesh);
						ItemFractureCollection = MetaData->StaticMeshAssetData.FractureCollection;
						ItemFractureCache = MetaData->StaticMeshAssetData.FractureCache;
						ItemMesh->SetRelativeTransform(MetaData->StaticMeshAssetData.StaticMeshTransform);
					}
					if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(ItemMesh))
					{
						SkeletalMeshComp->SetSkeletalMesh(MetaData->SkeletalMeshAssetData.SkeletalMesh);
						SkeletalMeshComp->SetAnimClass(MetaData->SkeletalMeshAssetData.AnimClass);
						ItemMesh->SetRelativeTransform(MetaData->SkeletalMeshAssetData.SkeletalMeshTransform);
					}
				}
			}
		}

		if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(GetClass()))
		{
			if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape)
				&& IsValid(ItemCollision))
			{
				FNAEdItemActorEditorUtils::UpdateSCSParentComponentReference(
					BPGC, TEXT("ItemCollision"), ItemCollision->GetFName());
			}
			if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshType)
				&& IsValid(ItemMesh))
			{
				FNAEdItemActorEditorUtils::UpdateSCSParentComponentReference(
					BPGC, TEXT("ItemMesh"), ItemMesh->GetFName());
			}
			/*TArray<USCS_Node*> SCSNodes = BPGC->SimpleConstructionScript->GetAllNodes();
			if (SCSNodes.Num() > 0)
			{
				for (USCS_Node* SCSNode : SCSNodes)
				{
					FName ParentComponentName = SCSNode->ParentComponentOrVariableName;

					if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_CollisionShape))
					{
						if (ParentComponentName.ToString().StartsWith(TEXT("ItemCollision"))
							&& !ParentComponentName.IsEqual(ItemCollision->GetFName()))
						{
							SCSNode->ParentComponentOrVariableName = ItemCollision->GetFName();
						}
					}
					if (EnumHasAnyFlags(DirtyFlags, EItemSubobjDirtyFlags::ISDF_MeshType))
					{
						if (ParentComponentName.ToString().StartsWith(TEXT("ItemMesh"))
							&& !ParentComponentName.IsEqual(ItemMesh->GetFName()))
						{
							SCSNode->ParentComponentOrVariableName = ItemMesh->GetFName();
						}
					}
				}
			}*/
		}
	}
}

void ANAItemActor::EnsureForceNonDataOnlyVariableUsed()
{
	if (!UNAItemEngineSubsystem::Get())
	{
		UE_LOG(NAItem, Warning,
		       TEXT("[%hs] 아이템 엔진 서브시스템 미초기화 상태"), __FUNCTION__);
		return;
	}
	if (!FNAEdItemBridge::IsRegisteredItemMetaClass(GetClass()))
	{
		UE_LOG(NAItem, Warning,
			TEXT("[%hs] 비등록 아이템 클래스에서 호출됨"), __FUNCTION__);
		return;
	}
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(NAItem, Warning,
			TEXT("[%hs] 잘못된 호출: CDO 외 객체에서 호출됨"), __FUNCTION__);
		return;
	}
	if (GetWorld() && GetWorld()->IsPlayInEditor())
	{
		UE_LOG(NAItem, Warning,
			TEXT("[%hs] 잘못된 호출: PIE 실행 중 호출됨"), __FUNCTION__);
		return;
	}
	UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(GetClass()));
	if (!BP)
	{
		UE_LOG(NAItem, Warning, TEXT("[%hs] 블루프린트 클래스 아님"), __FUNCTION__);
		return;
	}
	if (!bForceNonDataOnlyBlueprint)
	{
		FProperty* Prop = FindFProperty<FProperty>(
			GetClass()
			, GET_MEMBER_NAME_CHECKED(ANAItemActor, bForceNonDataOnlyBlueprint));
		bForceNonDataOnlyBlueprint
			= FNAEdItemActorEditorUtils::EnsureForceNonDataOnlyVariableAdded(
				BP, Prop);
	}
	
	/*
	if (BP->bRunConstructionScriptOnDrag)
	{
		BP->bRunConstructionScriptOnDrag = false;
	}

	if (!bForceNonDataOnlyBlueprint)
	{
		TArray<UEdGraph*> Graphs;
		BP->GetAllGraphs(Graphs);
		if (Graphs.Num() > 0)
		{
			for (UEdGraph* Graph : Graphs)
			{
				if (Graph->GetName().Equals(TEXT("EventGraph")))
				{
					bool bShouldAddDummyNode = true;
					TArray<UEdGraphNode*> GNodes = Graph->Nodes;
					if (GNodes.Num() > 0)
					{
						for (UEdGraphNode* GNode : GNodes)
						{
							if (UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(GNode))
							{
								bShouldAddDummyNode
									= !VarNode->GetVarNameString().Equals(TEXT("bForceNonDataOnlyBlueprint"));
								if (!bShouldAddDummyNode) break;
							}
						}
					}
					if (bShouldAddDummyNode)
					{
						FProperty* Property = nullptr;
						for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
						{
							if ((*It)->GetNameCPP().Equals(TEXT("bForceNonDataOnlyBlueprint")))
							{
								Property = *It;
								break;
							}
						}
						if (Property)
						{
							UBlueprintVariableNodeSpawner* GetterSpawner
								= UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(
									UNAEdHideableGraphNode_VariableGet::StaticClass()
									, Property);
							check(GetterSpawner != nullptr);
							UEdGraphNode* NewGetterNode = GetterSpawner->Invoke(
								Graph
								, IBlueprintNodeBinder::FBindingSet()
								, FVector2D(0.f, -10.f));
							if (INAEdHideableGraphNodeInterface* HideableGetterNode
								= Cast<INAEdHideableGraphNodeInterface>(NewGetterNode))
							{
								HideableGetterNode->SetHiddenFromEditor(true);
							}
							bForceNonDataOnlyBlueprint = NewGetterNode ? true : false;
						}
					}
					else
					{
						bForceNonDataOnlyBlueprint = true;
					}
				}
			}
		}
		if (bForceNonDataOnlyBlueprint)
		{
			BP->MarkPackageDirty();
		}
	}*/
}
#endif

void ANAItemActor::OnConstruction(const FTransform& Transform)
{
 	Super::OnConstruction(Transform);
	
#if WITH_EDITOR
	ReconstructItemSubobjectsFromMetaData();
#endif
	InitItemData();
}

void ANAItemActor::InitItemData()
{
	// CDO 또는 프리뷰 액터 또는 Child Actor인 경우: 새로운 아이템 데이터 인스턴스 생성 안함
	if (HasAnyFlags(RF_ClassDefaultObject)
		|| (GetWorld() && GetWorld()->IsPreviewWorld()) || IsChildActor()) return;
	if (HasValidItemID()) return;
	
	if (const UNAItemData* NewItemData = UNAItemEngineSubsystem::Get()->CreateItemDataByActor(this))
	{
		ItemDataID = NewItemData->GetItemID();
		VerifyInteractableData();
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

void ANAItemActor::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);
#if WITH_EDITOR
	if (UNAItemEngineSubsystem::Get()
		&& FNAEdItemBridge::IsRegisteredItemMetaClass(GetClass()))
	{
		if (HasAnyFlags(RF_ClassDefaultObject)
			&& GetClass()->HasAllClassFlags(CLASS_CompiledFromBlueprint)
			&& !SaveContext.IsProceduralSave())
		{
			BackupItemSubobjectPropertiesToMetaData();
			FNAEdItemBridge::SaveMetaDataTable(GetClass());
		}
	}
#endif
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

void ANAItemActor::MigrateItemStateFromChildActor(ANAItemActor* SourceChildActor, ANAItemActor* TargetActor)
{
	if ( UNAItemEngineSubsystem::Get() && SourceChildActor && TargetActor)
	{
		if (ensureAlwaysMsgf(SourceChildActor->IsChildActor() && SourceChildActor->HasValidItemID()
		                     && !TargetActor->IsChildActor() && TargetActor->HasValidItemID(),
		                     TEXT(
			                     "[MigrateItemStateFromChildActor]  ")))
		{
			if (UNAItemEngineSubsystem::Get()->DestroyRuntimeItem(TargetActor->ItemDataID))
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

void ANAItemActor::MigrateItemStateToChildActor(ANAItemActor* SourceActor, ANAItemActor* TargetChildActor)
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

void ANAItemActor::AssignItemDataToChildActor(UNAItemData* ItemData, ANAItemActor* TargetChildActor)
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
