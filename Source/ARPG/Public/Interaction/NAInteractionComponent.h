// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/NAWeakInteractableHandle.h"
#include "NAInteractionComponent.generated.h"


class INAInteractableInterface;
class ANAItemActor;

USTRUCT(BlueprintType)
struct FNAInteractionCandidateData
{
	GENERATED_BODY()

	UPROPERTY()
	float Distance = 0.0f;

	UPROPERTY()
	FVector Direction = FVector::ZeroVector;

	UPROPERTY()
	float DotWithView = -1.0f;

	UPROPERTY()
	float AngleDegrees = 0.f;
	
	UPROPERTY()
	bool bIsInViewAngle = false;
};

USTRUCT(BlueprintType)
struct FNAInteractionData
{
	GENERATED_BODY()
public:
	FNAInteractionData()
		: InteractorActor(nullptr)
		, InteractableItemActor(nullptr)
	{
	}

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Data")
	TWeakObjectPtr<AActor> InteractorActor;
	
	UPROPERTY(BlueprintReadOnly, Category = "Interaction Data")
	TWeakObjectPtr<ANAItemActor> InteractableItemActor;

	// 마지막 상호작용 확인이 이루어진 월드 시간(초)
	UPROPERTY(BlueprintReadOnly, Category = "Interaction Data")
	float LastInteractionCheckTime = 0.f;

	// Interactor와 Interactable 사이의 거리 및 방향 정보
	UPROPERTY(BlueprintReadOnly, Category = "Interaction Data")
	FNAInteractionCandidateData InteractionCandidateData;

	// 우선순위 비교용 점수 (Distance와 Dot을 조합한 정렬용 메타값)
	UPROPERTY(BlueprintReadOnly, Category = "Interaction Data")
	float InteractionScore = -FLT_MAX;
	
	bool IsValid() const
	{
		return InteractorActor.IsValid() && InteractableItemActor.IsValid();
	}

	void SetInteractionCandidateData(float Distance, FVector Direction, float DotWithView, float AngleDegrees, bool bIsInViewAngle)
	{
		InteractionCandidateData.Distance = Distance;
		InteractionCandidateData.Direction = Direction;
		InteractionCandidateData.DotWithView = DotWithView;
		InteractionCandidateData.AngleDegrees = AngleDegrees;
		InteractionCandidateData.bIsInViewAngle = bIsInViewAngle;
	}
	
	float CalculateInteractionScore()
	{
		// DotWithView: 1.0(정면), 0.0(측면), -1.0(정반대)
		// 정면 우선, 거리 보정. (가중치 조정 가능)
		const float AngleWeight = 10000.0f;
		const float DistWeight = 1.0f;
		// 시야각 밖은 점수 낮게
		if (!InteractionCandidateData.bIsInViewAngle)
		{
			InteractionScore = -FLT_MAX;
		}
		else
		{
			// 가까운 거리에, 정면에 가까울수록 점수↑
			InteractionScore =
				(InteractionCandidateData.DotWithView * AngleWeight) - (InteractionCandidateData.Distance * DistWeight);
		}
		return InteractionScore;
	}
};

UENUM(BlueprintType)
enum class ENAInteractionStatus : uint8
{
	IxS_None				UMETA(DisplayName = "None"),
	
	IxS_Succeeded			UMETA(DisplayName = "Succeeded"),
	//IxS_PartiallySucceeded	UMETA(DisplayName = "Partially Succeeded"),
	IxS_Failed				UMETA(DisplayName = "Failed"),
	IxS_Pended				UMETA(DisplayName = "Pended"),
};

UENUM(BlueprintType)
enum class ENAInteractionFailureReason : uint8
{
	IxFR_None					UMETA(DisplayName = "None"),
	
	IxFR_Canceled				UMETA(DisplayName = "Canceled"),				// 유저가 직접 취소
	IxFR_Interrupted			UMETA(DisplayName = "Interrupted"),				// 외부 방해로 중단
	IxFR_InvalidTarget			UMETA(DisplayName = "Invalid Target"),			// 상호작용 대상이 유효하지 않음
	IxFR_RequirementNotMet		UMETA(DisplayName = "Requirement Not Met"),		// 조건 부족 (인벤토리 여유 없음, 양 손 꽉 참 등)
	IxFR_OutOfRange				UMETA(DisplayName = "Out of Range"),			// 거리 부족
	IxFR_Unknown				UMETA(DisplayName = "Unknown"),					// 알 수 없는 이유
};

USTRUCT(BlueprintType)
struct FNAInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Result")
	ENAInteractionStatus OperationResult = ENAInteractionStatus::IxS_None;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Result")
	ENAInteractionFailureReason FailureReason = ENAInteractionFailureReason::IxFR_None;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Result")
	FText ResultMessage = FText::GetEmpty();

	static FNAInteractionResult InteractionSucceeded(const FText& SucceededText)
	{
		FNAInteractionResult SucceededResult;
		SucceededResult.OperationResult = ENAInteractionStatus::IxS_Succeeded;
		SucceededResult.FailureReason = ENAInteractionFailureReason::IxFR_None;
		SucceededResult.ResultMessage = SucceededText;

		return SucceededResult;
	}

	// static FNAInteractionResult InteractionPartiallySucceeded(const FText& SucceededText)
	// {
	// 	FNAInteractionResult PartiallySucceededResult;
	// 	PartiallySucceededResult.OperationResult = ENAInteractionStatus::IxS_PartiallySucceeded;
	// 	PartiallySucceededResult.FailureReason = ENAInteractionFailureReason::IxFR_None;
	// 	PartiallySucceededResult.ResultMessage = SucceededText;
	//
	// 	return PartiallySucceededResult;
	// }
	
	static FNAInteractionResult InteractionFailed(ENAInteractionFailureReason FailureReason, const FText& FailedText)
	{
		FNAInteractionResult FailedResult;
		FailedResult.OperationResult = ENAInteractionStatus::IxS_Failed;
		FailedResult.FailureReason = FailureReason;
		FailedResult.ResultMessage = FailedText;

		return FailedResult;
	}

	static FNAInteractionResult InteractionPended(const FText& PendedText)
	{
		FNAInteractionResult PendedResult;
		PendedResult.OperationResult = ENAInteractionStatus::IxS_Pended;
		PendedResult.FailureReason = ENAInteractionFailureReason::IxFR_None;
		PendedResult.ResultMessage = PendedText;

		return PendedResult;
	}
};

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

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
//======================================================================================================================
// Interaction Properties
//======================================================================================================================
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction Component")
	TMap<FWeakInteractableHandle, FNAInteractionData> FocusedInteractables;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Component",
		meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float InteractionAngleDegrees = 170.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Component")
	float InteractionCheckFrequency = 0.1f;

	float CurrentInteractionCheckTime;
	
	// 상호작용 트리거 시 딜레이가 필요하면 쓰기
	FTimerHandle InteractionTimerHandle;

	// 틱에서 UpdateInteractionData 실행 여부
	uint8 bUpdateInteractionData : 1 = false;

private:
	// 상호작용이 활성 중인(+사용 대기 상태) Interactable 객체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction Component", meta = (AllowPrivateAccess = "true"))
	FWeakInteractableHandle ActiveInteractable = nullptr;

	uint8 bHasPendingInteractable :1 = false;

	// 가장 가까이 있는 Interactable 객체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction Component", meta = (AllowPrivateAccess = "true"))
	FWeakInteractableHandle NearestInteractable = nullptr;
	
	uint8 bIsInInteraction :1 = false;

public:
//======================================================================================================================
// Interaction Functions
//======================================================================================================================

	/**
	 * @param InteractableActor 
	 * @return 최초 발견일 때만 true
	 */
	bool OnInteractableFound(TScriptInterface<INAInteractableInterface> InteractableActor);
	bool OnInteractableLost(TScriptInterface<INAInteractableInterface> InteractableActor);

	void ToggleInteraction();

	bool HasPendingInteractable() const
	{
		const bool b2 = ActiveInteractable.IsValid();
		const bool b3 = ActiveInteractable.ToScriptInterface() ?
		ActiveInteractable.ToScriptInterface()->IsAttachedAndPendingUse() : false;
		
		return bHasPendingInteractable && b2 && b3;
	}

	TScriptInterface<INAInteractableInterface> GetCurrentActiveInteractable() const
	{
		return ActiveInteractable.IsValid() ?  ActiveInteractable.GetRawObject() : nullptr;
	}

	/**
	 * @param InteractableActor	어태치에 성공하면 파괴됨
	 * @return ChildActorComponent에 의해 새로 생성된 객체.
	 */
	TScriptInterface<INAInteractableInterface> TryAttachItemMeshToOwner(TScriptInterface<INAInteractableInterface> InteractableActor);
	
protected:
	void UpdateInteractionData();
	void SetNearestInteractable(FWeakInteractableHandle InteractableActor);
	void SetActiveInteractable(TScriptInterface<INAInteractableInterface> InteractableActor);

	// 캐릭터에서 상호작용 시작 이니시
	FNAInteractionResult StratInteraction(FWeakInteractableHandle TargetInteractable);

	// 캐릭터에서 상호작용 중단 이니시
	void StopInteraction(FWeakInteractableHandle TargetInteractable);
	
	void MigrateInteractionData(FWeakInteractableHandle SourceInteractableHandle, FWeakInteractableHandle TargetInteractableHandle);
	
public:
//======================================================================================================================
// 인벤토리 연계
//======================================================================================================================

	/**
	 * @param ItemActor 
	 * @return	0: 전부 추가 성공
	 *			0>: 부분 추가
	 *			0<: 전부 실패
	 */
	int32 TryAddItemToInventory(ANAItemActor* ItemActor);
};