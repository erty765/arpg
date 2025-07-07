#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Interaction/NAInteractableDataStructs.h"
#include "NAItemBaseDataStructs.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IT_None					UMETA(Hidden), // 초기화 필요

	IT_Weapon				UMETA(DisplayName = "Weapon"),
	IT_Tool					UMETA(DisplayName = "Tool"),
	IT_Armor				UMETA(DisplayName = "Armor"),

	IT_Ammo					UMETA(DisplayName = "Ammo"),
	IT_Consumable			UMETA(DisplayName = "Consumable"),
	
	//IT_WeaponEnhancement	UMETA(DisplayName = "Weapon Upgrade Material"),
	//IT_ArmorEnhancement	UMETA(DisplayName = "Armor Upgrade Material"),

// Currency Item ///////////////////////////////////////////////////////////////////////////////////////////////////////

	IT_PowerNode			UMETA(DisplayName = "Power Node"),
	IT_Credit				UMETA(DisplayName = "Credit"),
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	IT_Material				UMETA(DisplayName = "Material"),
	IT_QuestItem			UMETA(DisplayName = "Quest Item"),
	
	IT_Misc					UMETA(DisplayName = "Miscellaneous"),

	IT_EnvObject			UMETA(DisplayName = "Environment Object"),
};

USTRUCT()
struct FItemTextData
{
	GENERATED_BODY()

	// RowName == ItemName(소문자-대문자 사이 공백 추가)
	UPROPERTY(VisibleAnywhere, Category = "Item Text Data")
	FText Name = FText::GetEmpty();

	UPROPERTY(EditAnywhere, Category = "Item Text Data")
	FText Description = FText::GetEmpty();

	UPROPERTY(EditAnywhere, Category = "Item Text Data")
	FText UsageText = FText::GetEmpty();
};

class UGeometryCollection;
class UGeometryCollectionCache;
USTRUCT()
struct ARPG_API FNAStaticMeshItemAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Static Mesh Item Asset Data")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	// (선택) Fracture Geometry Collection 에셋
	UPROPERTY(EditAnywhere, Category = "Static Mesh Item Asset Data|Fracture")
	UGeometryCollection* FractureCollection = nullptr;

	// (선택) Fracture Geometry Collection 애니메이션 
	UPROPERTY(EditAnywhere, Category = "Static Mesh Item Asset Data|Fracture")
	UGeometryCollectionCache* FractureCache = nullptr;
};

USTRUCT()
struct FNASkeletalMeshItemAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Skeletal Mesh Item Asset Data")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	// @TODO: 아이템 전용 애님 클래스 만들기?
	UPROPERTY(EditAnywhere, Category = "Skeletal Mesh Item Asset Data")
	TSubclassOf<UAnimInstance/*UItemAnimInstance*/> AnimClass;
};

UENUM(BlueprintType)
enum class EItemCollisionShape : uint8
{
	ICS_None		UMETA(DisplayName = "None"),
	
	ICS_Sphere		UMETA(DisplayName = "Sphere"),
	ICS_Box			UMETA(DisplayName = "Box"),
	ICS_Capsule		UMETA(DisplayName = "Capsule"),
};

UENUM(BlueprintType)
enum class EItemMeshType : uint8
{
	IMT_None		UMETA(DisplayName = "None"),
	
	IMT_Static		UMETA(DisplayName = "Static"),
	IMT_Skeletal	UMETA(DisplayName = "Skeletal"),
};


/**
 * 무기의 경우: bIsStackable이 true면 무조건 MaxSlotStackSize == 1 && MaxInventoryHoldCount == 1
 *			 bIsStackable이 false면 무조건 MaxSlotStackSize == -1 && MaxInventoryHoldCount == -1
*/
USTRUCT()
struct FItemNumericData
{
	GENERATED_BODY()

	/**
	 * 인벤토리의 슬롯 1칸에 들어갈 수 있는 최대 수량
	 * 1이면 인벤토리 슬롯 1칸에 1개만 들어감
	 * 주의! MaxSlotStackSize은  MaxInventoryStackSize보다 큰 값을 가질 수 없음!
	 */
	UPROPERTY(EditAnywhere, Category = "Item Numeric Data", meta = (UIMin = 1, UIMax = 999))
	int32 MaxSlotStackSize = -1;

	/**
	* 인벤토리에 소지 가능한 최대 수량
	* 1이면 인벤토리에 1개만 들어감, 0이면 수량 제한 없음
	*/
	UPROPERTY(EditAnywhere, Category = "Item Numeric Data", meta = (UIMin = 0))
	int32 MaxInventoryHoldCount = -1;
	
	UPROPERTY(EditAnywhere, Category = "Item Numeric Data")
	uint8 bIsStackable : 1 = false;	// 인벤토리에 슬롯에 스택이 가능한지
};

USTRUCT()
struct FNAIconAssetData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Item Icon Asset Data")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditAnywhere, Category = "Item Icon Asset Data")
	TObjectPtr<UTexture2D> IxButtonIcon = nullptr;
};

class ANAItemActor;
USTRUCT()
struct ARPG_API FNAItemBaseTableRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category ="Item Base Data")
	EItemType ItemType = EItemType::IT_None;

	// meta=(BlueprintBaseOnly): 블루프린트 클래스만 노출하도록 강제하는 지정자인데, 5.6 미만에서 버그 있음(언리얼피셜)
	// https://forums.unrealengine.com/t/uproperty-specifier-blueprintbaseonly-is-not-working-this-is-clearly-bug/2334795/6
	// https://issues.unrealengine.com/issue/UE-210088
	// 휴먼 에러 주의(c++ 네이티브 클래스 선택하지 말 것)
	UPROPERTY(EditAnywhere, Category = "Item Base Data", meta=(BlueprintBaseOnly, AllowAbstract="false"))
	TSoftClassPtr<ANAItemActor> ItemClass = nullptr;
	
	// None: 기본 생성자에서 DoNotCreateSubobject로 ItemCollision 생성 수정한 경우!! 반다시!! 이 플래그 써야함
	UPROPERTY(EditAnywhere, Category = "Item Collision Shape")
	EItemCollisionShape CollisionShape = EItemCollisionShape::ICS_None;
	
	UPROPERTY(EditAnywhere, Category = "Item Collision Shape",
		meta=(EditCondition="CollisionShape==EItemCollisionShape::ICS_Sphere", EditConditionHides,ClampMin= "0.0"))
	float CollisionSphereRadius = 0.f;

	UPROPERTY(EditAnywhere, Category = "Item Collision Shape",
			meta=(EditCondition="CollisionShape==EItemCollisionShape::ICS_Box", EditConditionHides, ClampMin= "0.0"))
	FVector CollisionBoxExtent = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Item Collision Shape",
			meta=(EditCondition="CollisionShape==EItemCollisionShape::ICS_Capsule", EditConditionHides, ClampMin= "0.0"))
	FVector2D CollisionCapsuleSize = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Item Collision Shape",
		meta=(EditCondition="CollisionShape!=EItemCollisionShape::ICS_None", EditConditionHides))
	FTransform CollisionTransform = FTransform::Identity;
	
	/* ANAItemActor의 메쉬 타입*/
	// None: 기본 생성자에서 DoNotCreateSubobject로 ItemMesh 생성 수정한 경우!! 반다시!! 이 플래그 써야함
	UPROPERTY(EditAnywhere, Category = "Item Mesh")
	EItemMeshType MeshType = EItemMeshType::IMT_Static;
	
	/** Static Mesh 에셋*/
	UPROPERTY(EditAnywhere, Category = "Item Mesh",
		meta=(EditCondition="MeshType==EItemMeshType::IMT_Static", EditConditionHides))
	FNAStaticMeshItemAssetData StaticMeshAssetData;
	
	/** Skeletal  Mesh  에셋 */
	UPROPERTY(EditAnywhere, Category = "Item Mesh",
		meta=(EditCondition="MeshType==EItemMeshType::IMT_Skeletal", EditConditionHides))
	FNASkeletalMeshItemAssetData SkeletalMeshAssetData;
	
	UPROPERTY(EditAnywhere, Category = "Item Mesh",
		meta=(EditCondition="MeshType!=EItemMeshType::IMT_None", EditConditionHides))
	FTransform MeshTransform = FTransform::Identity;
	
	UPROPERTY(EditAnywhere, Category ="Item Icon")
	FNAIconAssetData IconAssetData;
	
	UPROPERTY(EditAnywhere, Category = "Item Text", meta=(ShowOnlyInnerProperties))
	FItemTextData TextData;

	UPROPERTY(EditAnywhere, Category = "Item Numeric Statics", meta=(ShowOnlyInnerProperties))
	FItemNumericData NumericData;
	
	UPROPERTY(EditAnywhere,/* Category = "Item Interactable Data", */meta=(ShowOnlyInnerProperties))
	FNAInteractableData InteractableData;
	
#if WITH_EDITOR
protected:
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
#endif
};

USTRUCT()
struct FNADropItemPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MonsterDropTable")
	TSubclassOf<ANAItemActor> ItemClasses;

	UPROPERTY(EditAnywhere, Category = "MonsterDropTable")
	float Probability;
};

// 몬스터당 하나씩		-> monster Status 쪽에 만들어 놓음
USTRUCT()
struct ARPG_API FNATestDropBaseTableRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Item Base Data", meta=(BlueprintBaseOnly, AllowAbstract="false"))
	TArray<FNADropItemPair> ItemClass;

};

// 우리의 목적: 강화하기

// 강화를 하기 위해서 -> 강화 아이템을 떨궈야겠지?
// 강화 아이템을 떨구기 위해선 몬스터가 드랍을 해야겠지?
// 강화 아이템을 뭘 드랍할지 테이블로 정리
// 몹이 죽으면 테이블 기반으로 확률적으로 돌려서 아이템 뽑고 SpawnActor 후 사망ㄴ

// ----------

// 강화 아이템 드랍 -> 인벤토리로 이동
// 위젯에서 인벤토리 기반으로 강화 아이템 띄워주고
// 위젯에서 강화 아이템이 충분히 있으면 강화 가능, 강화 요청 수행

// ----------

// 강화 => Ability 추가, (예: 중력 줍기)
// 위젯에서 강화 -> 강화에서 노드 강화를 하면
// 노드 강화 -> GAS에서 캡처 -> Ability 부여
