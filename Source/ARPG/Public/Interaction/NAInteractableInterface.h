// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interaction/NAInteractableDataStructs.h"
#include "NAInteractableInterface.generated.h"

struct FNAItemBaseTableRow;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UNAInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 객체(아이템, 오브젝트) 전용 인터페이스
 * 상호작용 주체(캐릭터)는 UNAInteractionComponent를 쓰셈
 */
class ARPG_API INAInteractableInterface
{
	GENERATED_BODY()

public:
//====================================================================================================================================
// 순수 가상 메서드
//====================================================================================================================================

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	FNAInteractableData GetInteractableData() const;
	virtual bool GetInteractableData_Internal(FNAInteractableData& OutInteractableData) const =0;
	

	// @TODO: 아이템 메타데이터를 non const로 받아와서 InteractableData에 직접 대입함. 아이템 메타 데이터의 무결성을 침해할 우려가 있음
	// @TODO: 아이템 인터렉터블 데이터를 아이템 메타데이터처럼 취급? 아이템 인터렉터블 데이터가 런타임 중에 바뀔 일이 있나?
	// @TODO: 아이템 인터렉터블 데이터가 런타임 중에 변경될 일이 없다고 판단되면, 이 함수 지우기 & 인터렉터블 데이터 getter 함수 모두 const로 변경 
	// @TODO: 이 함수 제거할 때, 코드에서는 아이템 인터렉터블 데이터의 수정 여부(에디터에서의 수동 편집 포함)를 캐싱하는 플래그만 수정하도록 변경 
	// @see ANAItemActor::SetInteractableData
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void SetInteractableData(const FNAInteractableData& NewInteractableData);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool CanUseRootAsTriggerShape() const;
	
	
//====================================================================================================================================
// 기본 클래스(C++)에서 구현 시 INAInteractableInterface::..._Implementation 호출이 필요한 가상 메서드
//====================================================================================================================================

	// If overriding this function in a base class, **you must manually call INAInteractableInterface::CanInteract_Implementation()**
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool CanInteract() const;
	
	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::NotifyFocusBegin_Implementation()**
	* @note	이 함수를 해당 객체(AActor 파생)의 OnActorBeginOverlap에 바인딩하기
	* @note	호출 순서) INAInteractableInterface에서 자신이 포커스 되었음을 UNAInteractionComponent에 알림 -> UNAInteractionComponent에서 후속 처리
	* @param	InteractableActor:	상호작용 객체(ItemActor)
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void NotifyInteractableFocusBegin(AActor* InteractableActor, AActor* InteractorActor);

	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::NotifyFocusEnd_Implementation()**
	* @note	이 함수를 해당 객체(AActor 파생)의 OnActorEndOverlap에 바인딩하기
	* @note	호출 순서) INAInteractableInterface에서 자신이 포커스 해제되었음을 UNAInteractionComponent에 알림 -> UNAInteractionComponent에서 후속 처리
	* @param	InteractableActor:	상호작용 객체(ItemActor)
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void NotifyInteractableFocusEnd(AActor* InteractableActor, AActor* InteractorActor);

	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::BeginInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) 유저가 상호작용 시작 입력 시 UNAInteractionComponent에서 BeginInteraction 호출 -> INAInteractableInterface::BeginInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void BeginInteract(AActor* InteractorActor);

	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::EndInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) 유저가 상호작용 종료 입력 시 UNAInteractionComponent에서 EndInteraction 호출 -> INAInteractableInterface::EndInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void EndInteract(AActor* InteractorActor);

	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::ExecuteInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) UNAInteractionComponent에서 ExecuteInteraction 호출 -> INAInteractableInterface::ExecuteInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void ExecuteInteract(AActor* InteractorActor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool IsOnInteract() const;

protected:
	// If overriding this function in a base class, **you must manually call INAInteractableInterface::InitInteractableData_Implementation()**
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	void SetInteractableDataToBaseline(
		ENAInteractableType InteractableType,
		const FString& InteractionName,
		//const FString& InteractionScript,
		float InteractionDuration,
		int32 InQuantity);
	
//====================================================================================================================================
// c++에서만 호출 가능한 일반 메서드
//====================================================================================================================================
protected:
	// InteractableData가 맥락상의 기본값으로 편집되었는지 확인함 (에디터에서 수동으로 편집되었는지 여부를 반영하여 확인 결과를 반환)
	// baseline과 비교하는 과정에서 에디터 수동 편집 사항을 반영하기 위해 내부 캐시(bModifiedFromBaseline)를 갱신할 수 있음
	// 하지만 함수의 본질은 상태 탐지이며, 외부 사용자는 이를 판별 함수로 간주해도 무방함
	bool CheckInteractableEdit(const FNAInteractableData& InteractableData) const;
	const FNAInteractableData* TryGetInteractableData(const FNAItemBaseTableRow* InItemMetaData) const;
	void TrySetInteractableData(FNAItemBaseTableRow* InItemMetaData, const FNAInteractableData& NewInteractableData);
	void MarkBaselineModified(const FNAInteractableData& OutInteractableData);
	class UNAInteractionComponent* TryGetInteractionComponent(AActor* InActor);
	
protected:
	uint8 bIsFocused : 1 = false;
	uint8 bIsOnInteract : 1 = false;
};