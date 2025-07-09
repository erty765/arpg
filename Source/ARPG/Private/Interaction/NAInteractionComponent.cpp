// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/NAInteractionComponent.h"

#include "NACharacter.h"
#include "Item/ItemActor/NAItemActor.h"
#include "Inventory/Component/NAInventoryComponent.h"
#include "Item/ItemActor/NAPickableItemActor.h"
#include "Item/ItemActor/NAPlaceableItemActor.h"
#include "Item/PickableItem/NAWeapon.h"


// Sets default values for this component's properties
UNAInteractionComponent::UNAInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);

	SetIsReplicatedByDefault( true );

	// ...
}

void UNAInteractionComponent::OnRegister()
{
	Super::OnRegister();
	
}

// Called when the game starts
void UNAInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

// Called every frame
void UNAInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	CurrentInteractionCheckTime += DeltaTime;
	if (bUpdateInteractionData && CurrentInteractionCheckTime >= InteractionCheckFrequency)
	{
		UpdateInteractionData();
		CurrentInteractionCheckTime = 0.f;
	}
}

// 무기 어태치는 이 함수 쓰지 말것!
// 어태치되면서 기존 액터를 ChlidActorComponent가 생성한 객체로 대체
// ItemData & InteractionData & Interactable State 전부 인수인계함
TScriptInterface<INAInteractableInterface> UNAInteractionComponent::TryAttachItemMeshToOwner(
	TScriptInterface<INAInteractableInterface> InteractableActor)
{
	if (UObject* InteractableObjToAttach = InteractableActor.GetObject())
	{
		ANAItemActor* InteractableActorToAttach = Cast<ANAItemActor>(InteractableObjToAttach);
		if (!InteractableActorToAttach)
		{
			// 로그
			check( false );
			return nullptr;
		}

		if (InteractableActorToAttach->GetClass()->IsChildOf<ANAWeapon>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[TryAttachItemMeshToOwner]  무기 액터 어태치는 이 함수 쓰면 안됨!"));
			return nullptr;
		}
		
		if ( const TScriptInterface<INAHandActor> HandActor = GetOwner() )
		{
			FWeakInteractableHandle SourceInteractable = FWeakInteractableHandle(InteractableActor);
			if (!FocusedInteractables.Contains(SourceInteractable)) return nullptr;
			
			ANAItemActor* NewlyAttachedItemActor = nullptr;
			if (!HandActor->GetRightHandChildActorComponent()->GetChildActor())
			{
				HandActor->GetRightHandChildActorComponent()->SetChildActorClass(InteractableActorToAttach->GetClass());
				if (AActor* NewRightHandChildActor = HandActor->GetRightHandChildActorComponent()->GetChildActor())
				{
					NewlyAttachedItemActor = CastChecked<ANAItemActor>(NewRightHandChildActor);
				}
			}
			else if (!HandActor->GetLeftHandChildActorComponent()->GetChildActor())
			{
				HandActor->GetLeftHandChildActorComponent()->SetChildActorClass( InteractableActorToAttach->GetClass());
				if (AActor* NewLeftHandChildActor = HandActor->GetLeftHandChildActorComponent()->GetChildActor())
				{
					NewlyAttachedItemActor = CastChecked<ANAItemActor>(NewLeftHandChildActor);
				}
			}

			if (NewlyAttachedItemActor)
			{
				ANAItemActor::MigrateItemStateToChildActor(InteractableActorToAttach, NewlyAttachedItemActor);
				TScriptInterface<INAInteractableInterface> NewlyAttachedInteractableActor =
					NewlyAttachedItemActor->GetInteractableInterface();
				MigrateInteractionData(SourceInteractable, NewlyAttachedInteractableActor);
				SetActiveInteractable(NewlyAttachedInteractableActor);
				
				return NewlyAttachedInteractableActor;
			}
		}
	}

	return nullptr;
}

void UNAInteractionComponent::MigrateInteractionData(FWeakInteractableHandle SourceInteractableHandle, FWeakInteractableHandle TargetInteractableHandle)
{
	if (!TargetInteractableHandle.IsValid()) return;
	if (!FocusedInteractables.Contains(SourceInteractableHandle)) return;
	
	FNAInteractionData CachedInteractableData;
	FocusedInteractables.RemoveAndCopyValue(SourceInteractableHandle, CachedInteractableData);
	CachedInteractableData.InteractorActor = GetOwner();
	CachedInteractableData.InteractableItemActor = CastChecked<ANAItemActor>(TargetInteractableHandle.GetRawObject());
	if (CachedInteractableData.IsValid())
	{
		FocusedInteractables.Emplace(TargetInteractableHandle, CachedInteractableData);
	}
}

void UNAInteractionComponent::UpdateInteractionData()
{
	if (!GetOwner()) { return;}
	
	if (FocusedInteractables.IsEmpty())
	{
		bUpdateInteractionData = false;
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	TPair<FWeakInteractableHandle, float> BestInteractable(nullptr, -FLT_MAX);
	for ( auto It = FocusedInteractables.CreateIterator(); It; ++It )
	{
		if (!It->Key.IsValid() || !It->Value.IsValid())
		{
			//ensureAlways(false);
			It.RemoveCurrent();
			continue;
		}
		if (It->Value.InteractorActor.Get() != GetOwner())
		{
			//ensureAlways(false);
			It.RemoveCurrent();
			continue;
		}
		if (AActor* AttachParentActor = It->Value.InteractorActor->GetAttachParentActor())
		{
			//ensureAlways(AttachParentActor == GetOwner());
			if (AttachParentActor != GetOwner())
			{
				It.RemoveCurrent();
			}
			continue;
		}

		It->Value.LastInteractionCheckTime = Now;
		AActor* Owner = GetOwner();
		AActor* Interactable = It->Value.InteractableItemActor.Get();
		FVector OwnerForward = Owner->GetActorForwardVector();
		
		FVector OwnerLoc = Owner->GetActorLocation();
		FVector InteractableLoc = Interactable->GetActorLocation();

		float Distance = FVector::Dist(OwnerLoc, InteractableLoc);
		FVector Direction = (InteractableLoc - OwnerLoc).GetSafeNormal();
		
		FVector2D ToTarget2D = FVector2D(InteractableLoc - OwnerLoc).GetSafeNormal();
		FVector2D Forward2D = FVector2D(Owner->GetActorForwardVector()).GetSafeNormal();
		float DotWithView = FVector2D::DotProduct(Forward2D, ToTarget2D);
		
		float AngleDegrees = FMath::RadiansToDegrees(acosf(FMath::Clamp(DotWithView, -1.0f, 1.0f)));
		const bool bIsInViewAngle = FMath::IsWithinInclusive(AngleDegrees,
			- InteractionAngleDegrees * 0.5f, InteractionAngleDegrees * 0.5f);
		
		It->Value.SetInteractionCandidateData(Distance, Direction, DotWithView, AngleDegrees, bIsInViewAngle);
		const float InteractionScore = It->Value.CalculateInteractionScore();

		if (BestInteractable.Value < InteractionScore)
		{
			BestInteractable.Key = It->Key;
			BestInteractable.Value = InteractionScore;
		}
	}
	
	SetNearestInteractable(BestInteractable.Key);
}

void UNAInteractionComponent::SetNearestInteractable(FWeakInteractableHandle InteractableActor)
{
	if (NearestInteractable != InteractableActor)
	{
		NearestInteractable = InteractableActor;
	}
}

bool UNAInteractionComponent::OnInteractableFound(TScriptInterface<INAInteractableInterface> InteractableActor)
{
	if (!GetOwner()) return false;
	if (!InteractableActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnInteractableFound]  InteractableActor이 null이었음"));
		return false;
	}
	
	if (!FocusedInteractables.Contains(InteractableActor))
	{
		UObject* InteractableObj = InteractableActor.GetObject();
		if (!InteractableObj)
		{
			UE_LOG(LogTemp, Warning, TEXT("[OnInteractableFound]  InteractableObj가 유효하지 않음"));
			return false;
		}
		ANAItemActor* InteractableItem = Cast<ANAItemActor>(InteractableObj);
		if (!InteractableItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("[OnInteractableFound]  InteractableActor를 ANAItemActor로 캐스팅 실패"));
			return false;
		}
		AActor* AttachParentActor = InteractableItem->GetAttachParentActor();
		if (AttachParentActor && AttachParentActor != GetOwner())
		{
			UE_LOG(LogTemp, Log, TEXT("%hs: %s 아이템 주인이 이미 있음"), __FUNCTION__, *GetNameSafe(InteractableItem));
			return false;
		}
		
		FNAInteractionData NewInteractionData;
		NewInteractionData.InteractorActor = GetOwner();
		NewInteractionData.InteractableItemActor = InteractableItem;
		FocusedInteractables.Emplace(InteractableActor, NewInteractionData);

		if (!IsComponentTickEnabled())
		{
			SetComponentTickEnabled(true);
		}
		if (!bUpdateInteractionData)
		{
			bUpdateInteractionData = true;
		}
		return true;
	}

	return false;
}

bool UNAInteractionComponent::OnInteractableLost(TScriptInterface<INAInteractableInterface> InteractableActor)
{
	if (!InteractableActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnInteractableLost]  InteractableActor이 null이었음"));
		return false;
	}
	
	if (FocusedInteractables.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnInteractableLost]  FocusedInteractableMap이 비어있었는데??"));
		return false;
	}

	if (!FocusedInteractables.Contains(InteractableActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnInteractableLost]  FocusedInteractableMap에 없던 InteractableActor이었음"));
		return false;
	}

	if (HasPendingInteractable() && ActiveInteractable == InteractableActor)
	{
		return false;
	}

	int32 bSucceed = FocusedInteractables.Remove(InteractableActor);
	if (bSucceed != 1)
	{
		UE_LOG(LogTemp, Warning, TEXT(
			"[OnInteractableLost]  FocusedInteractableMap에서 InteractableActor 제거 실패: Removed %d"), bSucceed);
		return false;
	}

	if (ActiveInteractable == InteractableActor)
	{
		ActiveInteractable = nullptr;
	}
	if (NearestInteractable == InteractableActor)
	{
		NearestInteractable = nullptr;
	}

	return true;
}

void UNAInteractionComponent::ToggleInteraction()
{
	if (!NearestInteractable.IsValid()
		|| !FocusedInteractables.Contains(NearestInteractable))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToggleInteraction]  NearestInteractable이 유효하지 않음"));
		return;
	}
	if (!NearestInteractable.ToWeakInterface()->Execute_CanInteract(NearestInteractable.GetRawObject()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToggleInteraction]  NearestInteractable이 상호작용 가능한 상태가 아님"));
		return;
	}
	FNAInteractionData NearestInteractableData = FocusedInteractables[NearestInteractable];
	if (NearestInteractableData.InteractorActor != GetOwner())
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("[ToggleInteraction]  NearestInteractable이 대상으로 하는 interactor(%s)가 Owner(%s)와 다름.")
		       , NearestInteractableData.InteractorActor.IsValid()
					? *NearestInteractableData.InteractorActor->GetName()
					: TEXT_NULL
		       , GetOwner()
					? *GetOwner()->GetName()
					: TEXT_NULL);
		return;
	}
	
	bIsInInteraction = true;
	if (NearestInteractable.ToWeakInterface()->HasInteractionDelay())
	{
		float InteractionDelay = NearestInteractable.ToWeakInterface()->GetInteractionDelay();
		FWeakInteractableHandle CapturedNearestInteractable = NearestInteractable;
		if (InteractionDelay > 0.f)
		{
			// @TODO: 상호작용 트리거 딜레이 동안 ux 어떻게 처리할지 고민하기
			GetWorld()->GetTimerManager().ClearTimer(InteractionTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(
				InteractionTimerHandle,
				FTimerDelegate::CreateLambda(
					[this, CapturedNearestInteractable]()
					{
						FNAInteractionResult Result = StratInteraction(CapturedNearestInteractable);
						if (Result.OperationResult == ENAInteractionStatus::IxS_Failed
							&& Result.FailureReason == ENAInteractionFailureReason::IxFR_RequirementNotMet)
						{
							if (HasPendingInteractable())
							{
								StopInteraction(NearestInteractable);
							}
						}
						bIsInInteraction = false;
					}),
					InteractionDelay,
					false);
		}
	}
	else
	{
		FNAInteractionResult Result = StratInteraction(NearestInteractable);
		if (Result.OperationResult == ENAInteractionStatus::IxS_Failed
			&& Result.FailureReason == ENAInteractionFailureReason::IxFR_RequirementNotMet)
		{
			if (HasPendingInteractable())
			{
				StopInteraction(NearestInteractable);
			}
		}
		bIsInInteraction = false;
	}
}

FNAInteractionResult UNAInteractionComponent::StratInteraction(FWeakInteractableHandle TargetInteractable)
{
	if (!TargetInteractable.IsValid()
		|| !FocusedInteractables.Contains(TargetInteractable))
	{
		return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
			FText::FromString(TEXT(
				"Interaction failed: Target interactable is invalid or not currently focused.")));
	}

	if (!TargetInteractable.ToWeakInterface()->Execute_CanInteract(TargetInteractable.GetRawObject()))
	{
		return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
			FText::FromString(TEXT(
				"Interaction failed: Target interactable is not ready to interact.")));
	}

	FNAInteractionData TargetInteractableData = FocusedInteractables[TargetInteractable];
	if (TargetInteractableData.InteractorActor != GetOwner())
	{
		return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
		FText::Format(
			FText::FromString(TEXT(
				"Interaction target mismatch: Interactor('{0}') recorded by target differs from this owner('{1}')."))
			, TargetInteractableData.InteractorActor.IsValid()
				  ? FText::FromString(TargetInteractableData.InteractorActor->GetName())
				  : FText::FromString(TEXT_NULL)
			, GetOwner()
				  ? FText::FromString(GetOwner()->GetName())
				  : FText::FromString(TEXT_NULL)));
	}
	
	if (ANAPickableItemActor* PickableInteractable =
		Cast<ANAPickableItemActor>(TargetInteractableData.InteractableItemActor.Get()))
	{
		const EPickupMode PickupMode = PickableInteractable->GetPickupMode();
		if (PickupMode == EPickupMode::PM_None)
		{
			return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
			FText::FromString(TEXT(
				"Interaction failed: Target interactable's pickup mode is invalid.")));
		}
		if (EnumHasAnyFlags(PickupMode, EPickupMode::PM_AutoUse))
		{
			int32 AutoUseResult = PickableInteractable->TryPerformAutoUse(GetOwner());
			if (AutoUseResult == -1)
			{
				return FNAInteractionResult::InteractionSucceeded(FText::FromString(TEXT(
					"Item was automatically used and fully consumed.")));
			}
		}
		if (EnumHasAnyFlags(PickupMode, EPickupMode::PM_Inventory))
		{
			int32 AddResult = TryAddItemToInventory(PickableInteractable);
			if (AddResult == 0)
			{
				// 전부 추가 완료했으므로, 아이템 액터 파괴됨
				return FNAInteractionResult::InteractionSucceeded(FText::FromString(TEXT(
					"The item was added to your inventory and removed from the world.")));
			}
			else if (AddResult > 0)
			{
				// 남은 수량이 있으므로, 아이템 액터 보존
				return FNAInteractionResult::InteractionSucceeded(FText::FromString(TEXT(
					"Part of the item was added to your inventory. Remaining amount stays in the world.")));
			}
			else
			{
				// 전부 추가 실패
				return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_RequirementNotMet
					, FText::FromString(TEXT(
					"The item could not be added to your inventory. Requirements not met.")));
			}
		}
		if (EnumHasAnyFlags(PickupMode, EPickupMode::PM_CarryOnly))
		{
			if (PickableInteractable->GetClass()->IsChildOf<ANAWeapon>())
			{
				return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
					FText::FromString(TEXT(
						"")));
			}
			
			const TScriptInterface<INAHandActor> HandActor = GetOwner();
			ensureAlways(HandActor != nullptr);

			UChildActorComponent* RightHandChildActorComp = HandActor->GetRightHandChildActorComponent();
			UChildActorComponent* LeftHandChildActorComp = HandActor->GetLeftHandChildActorComponent();
			ensureAlways(RightHandChildActorComp && LeftHandChildActorComp);

			ANAItemActor* AttachedItemActor_Right = Cast<ANAItemActor>(RightHandChildActorComp->GetChildActor());
			ANAItemActor* AttachedItemActor_Left = Cast<ANAItemActor>(LeftHandChildActorComp->GetChildActor());
			
			if (!HasPendingInteractable())
			{
				ensureAlways(!AttachedItemActor_Right && !AttachedItemActor_Left);

				// A. 양 손이 비어 있다면, 어태치 시도
				TScriptInterface<INAInteractableInterface> NewInteractableActor =
					TryAttachItemMeshToOwner(TargetInteractable.ToScriptInterface());
				if (NewInteractableActor != nullptr)
				{
					// 어태치 성공하면, ChildActorComponent에 의해 새롭게 생성된 아이템 액터를 사용 대기 아이템으로 등록
					ensureAlways(ActiveInteractable.IsValid() && ActiveInteractable == NewInteractableActor);
					bHasPendingInteractable = true;
					NewInteractableActor->SetAttachedAndPendingUse(bHasPendingInteractable);
					return FNAInteractionResult::InteractionPended(FText::FromString(TEXT(
						"")));
				}
				else
				{
					// 어태치 실패
					return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_Unknown,
						FText::FromString(TEXT(
							"")));
				}
			}
			
			ensureAlways(ActiveInteractable == TargetInteractable);
			// B. 양 손이 비어 있지 않고, 어태치된 액터가 무기가 아니라면 -> 상호작용 가능 하면 begin interact
			if (TargetInteractable.ToScriptInterface()->CanPerformInteractionWith(GetOwner()))
			{
				if (PickableInteractable->Execute_TryInteract(PickableInteractable, GetOwner()))
				{
					// 상호작용 사이클 성공
					if (!PickableInteractable->IsUnlimitedInteractable()
						&& PickableInteractable->GetInteractableCount() <= 0)
					{
						// 상호작용 가능 횟수 == 0: 디태치 & 아이템 액터 파괴
						bool bDetached = false;
						if (AttachedItemActor_Right)
						{
							RightHandChildActorComp->DestroyChildActor();
							RightHandChildActorComp->SetChildActorClass(nullptr);
							bDetached = true;
						}
						else if (AttachedItemActor_Left)
						{
							LeftHandChildActorComp->DestroyChildActor();
							LeftHandChildActorComp->SetChildActorClass(nullptr);
							bDetached = true;
						}

						if (bDetached)
						{
							bHasPendingInteractable = false;
							OnInteractableLost(TargetInteractable.ToScriptInterface());
							return FNAInteractionResult::InteractionSucceeded(FText::FromString(TEXT(
								"상호작용 성공 후 상호작용 횟수 전부 소진됨. 아이템 액터 디태치 및 파괴")));
						}
					}
					else
					{
						return FNAInteractionResult::InteractionPended(FText::FromString(TEXT(
							"상호작용 성공")));
					}
				}
			}

			return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_RequirementNotMet,
			                                               FText::FromString(TEXT("조건 불충분")));
		}
	}
	else if (ANAPlaceableItemActor* PlaceableInteractable =
		Cast<ANAPlaceableItemActor>(TargetInteractableData.InteractableItemActor.Get()))
	{
		// 바로 begin interact
		if (TargetInteractable.ToScriptInterface()->CanPerformInteractionWith(GetOwner()))
		{
			SetActiveInteractable(TargetInteractable.ToScriptInterface());
			if (PlaceableInteractable->Execute_TryInteract(PlaceableInteractable, GetOwner()))
			{
				return FNAInteractionResult::InteractionSucceeded(
					FText::FromString(TEXT("상호작용 성공")));
			}
		}
		
		return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_RequirementNotMet,
				FText::FromString(TEXT("조건 불충분")));
	}

	return FNAInteractionResult::InteractionFailed(ENAInteractionFailureReason::IxFR_InvalidTarget,
		FText::FromString(TEXT("유효하지 않은 타겟(pickable or placeable이 아니었음)")));
}

void UNAInteractionComponent::StopInteraction(FWeakInteractableHandle TargetInteractable)
{
	if (HasPendingInteractable())
	{
		const TScriptInterface<INAHandActor> HandActor = GetOwner();
		ensureAlways(HandActor != nullptr);

		UChildActorComponent* RightHandChildActorComp = HandActor->GetRightHandChildActorComponent();
		UChildActorComponent* LeftHandChildActorComp = HandActor->GetLeftHandChildActorComponent();
		ensureAlways(RightHandChildActorComp && LeftHandChildActorComp);

		ANAItemActor* AttachedItemActor_Right = Cast<ANAItemActor>(RightHandChildActorComp->GetChildActor());
		ANAItemActor* AttachedItemActor_Left = Cast<ANAItemActor>(LeftHandChildActorComp->GetChildActor());
		ensureAlways(ActiveInteractable == TargetInteractable);
		
		ANAItemActor* ItemActorToDetach = AttachedItemActor_Right ? AttachedItemActor_Right : AttachedItemActor_Left;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = nullptr;
		SpawnParams.Template = ItemActorToDetach;
		SpawnParams.bNoFail = true;

		const FVector SpawnLocation = ItemActorToDetach->GetActorLocation();
		const FRotator SpawnRotation = FRotator::ZeroRotator;
		AActor* Spawned = GetWorld()->SpawnActor(GetClass(), &SpawnLocation, &SpawnRotation, SpawnParams);
		ANAItemActor* SpawnedItemActor = Cast<ANAItemActor>(Spawned);
		check(SpawnedItemActor);

		ANAItemActor::MigrateItemStateFromChildActor(ItemActorToDetach, SpawnedItemActor);
		
		bHasPendingInteractable = false;
		OnInteractableLost(TargetInteractable.ToScriptInterface());
	}
}

void UNAInteractionComponent::SetActiveInteractable(TScriptInterface<INAInteractableInterface> InteractableActor)
{
	if (ActiveInteractable != InteractableActor)
	{
		ActiveInteractable = InteractableActor;
	}
}

int32 UNAInteractionComponent::TryAddItemToInventory(ANAItemActor* ItemActor)
{
	UNAInventoryComponent* InventoryComp = GetOwner()->GetComponentByClass<UNAInventoryComponent>();
	if (!ensureAlways(InventoryComp)) return -1;
	
	if (!ItemActor) return -1;
	UNAItemData* Item = ItemActor->GetItemData();
	if (!Item) return -1;
	
	int32 RemainQuantity =  InventoryComp->TryAddItem(Item);
	if (RemainQuantity == 0) // 전부 추가 성공: 아이템 액터 파괴
	{
		UE_LOG(LogTemp, Warning, TEXT("[TryAddItemToInventory]  (%s) 전부 추가 성공"), *ItemActor->GetName());
		ItemActor->FinalizeAndDestroyAfterInventoryAdded(GetOwner());
	}
	else if (RemainQuantity > 0) // 부분 추가
	{
		// 아이템 액터 자체는 잔존하나, 아이템 데이터가 복제됨
		// 해당 아이템 액터에 남아있는 아이템 데이터 / 인벤토리에 적재된 아이템 데이터
		UE_LOG(LogTemp, Warning, TEXT("[TryAddItemToInventory]  (%s) 부분 추가: 남은 수량 (%d)"),
			*ItemActor->GetName(), RemainQuantity);
	}
	else if (RemainQuantity == -1) // 전부 실패
	{
		// 아이템 액터 잔존 및 아이템 데이터 변경 없음
		UE_LOG(LogTemp, Warning, TEXT("[TryAddItemToInventory]  (%s) 전부 추가 실패"), *ItemActor->GetName());
	}
	
	return RemainQuantity;
}
