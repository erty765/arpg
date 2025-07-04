#pragma once

#include "CoreMinimal.h"
#include "NAInteractableDataStructs.generated.h"

// UENUM(BlueprintType)
// enum class ENAInteractableType : uint8
// {
// 	Toggle	 UMETA(DisplayName = "Toggle"),	// 켜기/끄기
// 	Inspect	 UMETA(DisplayName = "Inspect"),// 살펴보기 (정보 확인용)
// 	Install	 UMETA(DisplayName = "Install"),// 설치 (예: 함정 설치)
// 	Remove	 UMETA(DisplayName = "Remove"),	// 제거 (설치물 제거 등)
// };

UENUM(BlueprintType)
enum class ENAInteractableType : uint8
{
	None	 UMETA(Hidden),				

	Pickup	 UMETA(DisplayName = "Pickup"),	// 줍기
	Toggle	 UMETA(DisplayName = "Toggle"),	// 켜기/끄기
	Inspect	 UMETA(DisplayName = "Inspect"),// 살펴보기 (정보 확인용)
	Install	 UMETA(DisplayName = "Install"),// 설치 (예: 함정 설치)
	Remove	 UMETA(DisplayName = "Remove"),	// 제거 (설치물 제거 등)
};

// 아이템 인스턴스 마다 구분되어야 하는 상호작용 데이터
USTRUCT(BlueprintType)
struct FNAInteractableData
{
	GENERATED_BODY()

public:
	FNAInteractableData()
		: InteractableType(ENAInteractableType::None),
		InteractionName(FText::GetEmpty()),
		InteractionDelayTime(0.f),
		InteractableCount(0),
		bIsUnlimitedInteractable(false)
	{
	}

	UPROPERTY(EditAnywhere, Category = "Item Interactable Data")
	ENAInteractableType InteractableType;

	UPROPERTY(VisibleAnywhere, Category = "Item Interactable Data")
	FText InteractionName;

	UPROPERTY(EditAnywhere, Category = "Item Interactable Data", meta=(ClampMin="0.0"))
	float InteractionDelayTime;

	// 상호작용 가능한 횟수: 단발성이면 1
	UPROPERTY(EditAnywhere, Category = "Item Interactable Data", meta=(ClampMin="0"))
	int32 InteractableCount;

	UPROPERTY(EditAnywhere, Category = "Item Interactable Data", meta=(ClampMin="0"))
	uint8 bIsUnlimitedInteractable : 1;
};