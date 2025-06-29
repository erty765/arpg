#include "Item/ItemActor/NAPickableItemActor.h"

#include "Components/SphereComponent.h"
#include "Interaction/NAInteractionComponent.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"

ANAPickableItemActor::ANAPickableItemActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ANAPickableItemActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}

void ANAPickableItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ANAPickableItemActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ANAPickableItemActor::BeginPlay()
{
	Super::BeginPlay();
}

int32 ANAPickableItemActor::TryPerformAutoUse(AActor* User)
{
	if (PickupMode != EPickupMode::PM_AutoUse) return 0;
	
	const int32 ConsumedQty = PerformAutoUse_Impl(User);
	if (ConsumedQty > 0)
	{
		int32 CurrQty = GetItemData()->GetQuantity();
		if (CurrQty - ConsumedQty <= 0)
		{
			// 자동 사용 후 아이템 다 씀 -> 이 액터 인스턴스 & 아이템 데이터 제거
			if (UNAItemEngineSubsystem::Get()->DestroyRuntimeItemData(GetItemData()))
			{
				Destroy();
				return -1;
			}
		}
		else
		{
			GetItemData()->SetQuantity(CurrQty - ConsumedQty);
		}
	}
	return ConsumedQty;
}

int32 ANAPickableItemActor::PerformAutoUse_Impl(AActor* User)
{
	return 0;
}

bool ANAPickableItemActor::CanInteract_Implementation() const
{
	return Super::CanInteract_Implementation() && PickupMode != EPickupMode::PM_None;
}

void ANAPickableItemActor::SetInteractionPhysicsEnabled(const bool bEnabled)
{
	if (!ensureAlways(Execute_IsOnInteract(this))) return;
	
	if (!bEnabled)
	{
		if (ItemCollision)
		{
			ItemCollision->SetSimulatePhysics(false);
			ItemCollision->SetGenerateOverlapEvents(false);
			ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemCollision->Deactivate();
		}
		if (TriggerSphere)
		{
			TriggerSphere->SetGenerateOverlapEvents(false);
			TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TriggerSphere->Deactivate();
		}
		
		CollapseItemWidgetComponent();
	}
	else
	{
		if (ItemCollision)
		{
			if ( HasAuthority() )
			{
				ItemCollision->SetSimulatePhysics(true);	
			}
			
			ItemCollision->SetGenerateOverlapEvents(true);
			ItemCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			ItemCollision->Activate();
		}
		if (TriggerSphere)
		{
			TriggerSphere->SetGenerateOverlapEvents(true);
			TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
			TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			TriggerSphere->Activate();
		}
	}
}

// void ANAPickableItemActor::EndInteract_Implementation(AActor* Interactor)
// {
// 	Super::EndInteract_Implementation(Interactor);
// 	
// 	UNAInteractionComponent* InteractComp = GetInteractionComponent(Interactor);
// 	if (!ensureAlways(IsValid(InteractComp))) return;
// 	
// 	// 아이템 드랍?
// 	// 아이템이 부착이 되어 있는 경우
// 	if ( GetParentActor() == Interactor )
// 	{
// 		FActorSpawnParameters SpawnParams;
// 		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
// 		SpawnParams.Owner = nullptr;
// 		SpawnParams.Template = this;
// 		SpawnParams.bNoFail = true;
// 	
// 		const FVector SpawnLocation = GetActorLocation();
// 		const FRotator SpawnRotation = FRotator::ZeroRotator;
// 		const AActor* Spawned = GetWorld()->SpawnActor( GetClass(), &SpawnLocation, &SpawnRotation, SpawnParams );
// 		check( Spawned );
// 	
// 		// ChildActorComponent에 의해 만들어진 액터인 경우
// 		// 부착되어 있던 아이템을 파괴 ( = 자신을 파괴 )
// 		if ( UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>( GetParentComponent() ) )
// 		{
// 			ChildActorComponent->DestroyChildActor();
// 			ChildActorComponent->SetChildActorClass( nullptr );
// 		}
// 		else
// 		{
// 			// 아니면 새로 스폰된 아이템에게 유산을 넘겨줌
// 			Destroy();
// 		}
// 	}
//
// 	if (!IsPendingKillPending())
// 	{
// 		TriggerSphere->SetGenerateOverlapEvents(true);
// 	}
// }
