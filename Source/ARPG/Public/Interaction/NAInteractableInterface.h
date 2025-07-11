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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool CanInteract() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool IsOnInteract() const;
	
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

	virtual bool TryGetInteractableData(FNAInteractableData& OutData) const { return false; }
	
	virtual bool HasInteractionDelay() const { return false; }
	virtual float GetInteractionDelay() const { return 0.0f; }

	virtual bool IsAttachedAndPendingUse() const { return false; }
	virtual void SetAttachedAndPendingUse(bool bNewState) {}

	virtual bool IsUnlimitedInteractable() const { return false; }
	virtual int32 GetInteractableCount() const { return -1; }
	virtual void SetInteractableCount(int32 NewCount) {}

	// 상호작용 실행 시도 시, 상호작용 실행 가능 조건이 충족되었는지 확인하는 함수
	// 내부에서 Interactable Type에 따라 필요한 조건 확인하기
	virtual bool CanPerformInteractionWith(AActor* Interactor) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool TryInteract(AActor* Interactor);
	
protected:
	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::BeginInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) 유저가 상호작용 시작 입력 시 UNAInteractionComponent에서 BeginInteraction 호출 -> INAInteractableInterface::BeginInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool BeginInteract(AActor* InteractorActor);

	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::ExecuteInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) UNAInteractionComponent에서 ExecuteInteraction 호출 -> INAInteractableInterface::ExecuteInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool ExecuteInteract(AActor* InteractorActor);
	
	/** If overriding this function in a base class, **you must manually call INAInteractableInterface::EndInteract_Implementation()**
	* @param	InteractorActor:	상호작용 대상자(캐릭터)
	* @note	호출 순서) 유저가 상호작용 종료 입력 시 UNAInteractionComponent에서 EndInteraction 호출 -> INAInteractableInterface::EndInteract
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactable Interface")
	bool EndInteract(AActor* InteractorActor);
	
	virtual void SetInteractionPhysicsEnabled(const bool bEnabled) {}
	
//====================================================================================================================================
// c++에서만 호출 가능한 일반 메서드
//====================================================================================================================================
protected:
	class UNAInteractionComponent* GetInteractionComponent(AActor* Interactor) const;

	static void TransferInteractableStateToChildActor(TScriptInterface<INAInteractableInterface> SourceActor,
		TScriptInterface<INAInteractableInterface> TargetChildActor)
	{
		if (SourceActor && TargetChildActor)
		{
			TargetChildActor->bIsFocused = SourceActor->bIsFocused;
			TargetChildActor->bIsOnInteract = SourceActor->bIsOnInteract;
		}
	}
	
protected:
	uint8 bIsFocused : 1 = false;
	uint8 bIsOnInteract : 1 = false;
	
	uint8 bIsAttachedAndPendingUse : 1 = false;
};