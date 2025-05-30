// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/NAWeakInteractableHandle.h"
#include "NAInteractionComponent.generated.h"

class ANAItemActor;

USTRUCT(BlueprintType)
struct FNAInteractionData
{
	GENERATED_BODY()
public:
	FNAInteractionData()
		: FocusedInteractable(nullptr),
		LastInteractionCheckTime(0.f),
		DistanceToActiveInteractable(0.f)
	{
	}

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interaction Data")
	TWeakObjectPtr<ANAItemActor> FocusedInteractable;

	// 마지막 상호작용 확인이 이루어진 월드 시간(초)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction Data")
	float LastInteractionCheckTime;

	// @TODO: FVector로 변경해서 거리 및 방향 정보도 저장할지 말지 생각해보기
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction Data")
	float DistanceToActiveInteractable;

	// 상호작용 트리거 시 딜레이가 필요하면 쓰기
	FTimerHandle InteractionTimerHandle;
};

class INAInteractableInterface;
struct FWeakInteractableKey;

/**
 * @TODO: 소유주의 트랜스폼 정보를 가져올 때, 소유주가 APawn인 경우 vs AController인 경우 나눠서 대응해야 함
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARPG_API UNAInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNAInteractionComponent();

protected:
	virtual void OnRegister() override;

	// Called when the game starts
	virtual void BeginPlay() override;

	void UpdateInteractionData();
	void SetNearestInteractable(INAInteractableInterface* InteractableActor);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AttachItemMeshToOwner(INAInteractableInterface* InteractableActor);

protected:
	//==================================================================================================
	// Interaction Properties
	//==================================================================================================

	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interaction Component")
	//TWeakInterfacePtr<class INAInteractableInterface> FocusedInteractable;

	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interaction Component")
	//FNAInteractionData InteractionData;

	UPROPERTY(Transient, DuplicateTransient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction Component")
	TMap<FWeakInteractableHandle, FNAInteractionData> FocusedInteractableMap;
	//TMap<TWeakInterfacePtr<class INAInteractableInterface>, FNAInteractionData> FocusedInteractableMap;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interaction Component")
	float InteractionCheckFrequency;

	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Interaction Component")
	//float InteractionCheckDistance;

	//FTimerHandle InteractionTimerHandle;

private:
	// 상호작용이 활성된 Interactable 객체
	UPROPERTY(Transient, DuplicateTransient)
	FWeakInteractableHandle ActiveInteractable = nullptr;
	//TWeakInterfacePtr<INAInteractableInterface> ActiveInteractable = nullptr;

	// 가장 가까이 있는 Interactable 객체
	UPROPERTY(Transient, DuplicateTransient)
	FWeakInteractableHandle NearestInteractable = nullptr;
	//TWeakInterfacePtr<INAInteractableInterface> NearestInteractable = nullptr;

public:
	//==================================================================================================
	// Interaction Functions
	//==================================================================================================

	// Interaction 가능한 아이템 인스턴스 쪽에서 상호작용 범위를 체크(트리거 콜리전 활용)
	// 아이템 인스턴스에서 유저가 상호작용 범위 내에 들어왔는지 체크 후 캐릭터에게 상호작용이 가능함을 알림
	bool OnInteractableFound(INAInteractableInterface* InteractableActor);
	// @TODO: FocusedInteractableMap 요소 지연 삭제 고민해보기 -> 상호작용 아이템 포커스 갱신 때문에 병목 생기는지 확인하기
	bool OnInteractableLost(INAInteractableInterface* InteractableActor);
	
	void BeginInteraction(/*INAInteractableInterface* InteractableActor*/);
	void EndInteraction(/*INAInteractableInterface* InteractableActor*/);
	void ExecuteInteraction(/*INAInteractableInterface* InteractableActor*/);

protected:
	uint8 bUpdateInteractionData : 1 = false;
};