// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PickableItem/NAMedPack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "HP/GameplayEffect/NAGE_Damage.h"
#include "HP/GameplayEffect/NAGE_Heal.h"
#include "Item/ItemDataStructs/NARecoveryPackDataStructs.h"
#include "NACharacter.h"
#include "NAPlayerState.h"


// Sets default values
ANAMedPack::ANAMedPack(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PickupMode = EPickupMode::PM_Inventory;
}

// Called when the game starts or when spawned
void ANAMedPack::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANAMedPack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ANAMedPack::CanUseItem(UNAItemData* InItemData, AActor* User) const
{
	ANACharacter* Character = Cast<ANACharacter>(User);
	if (!Character) return false;
	const ANAPlayerState* PlayerState = Character->GetPlayerState<ANAPlayerState>();
	if (!PlayerState) return false;

	int32 MaxHealth = PlayerState->GetMaxHealth();
	float CurrentHealth = PlayerState->GetHealth();
	
	return !FMath::IsNearlyEqual(CurrentHealth, MaxHealth);
}

bool ANAMedPack::UseItem(UNAItemData* InItemData, AActor* User, int32& UsedAmount) const
{
	Super::UseItem(InItemData, User,UsedAmount);

	if (!InItemData || !User) return false;
	if (InItemData->GetItemActorClass() != GetClass()) return false;

	if (const FNARecoveryPackTableRow* RecoveryPackData = InItemData->GetItemMetaDataStruct<FNARecoveryPackTableRow>())
	{
		float RecoveryAmount = RecoveryPackData->RecoveryAmount;
		if (RecoveryPackData->bIsPercentRecovery)
		{
			RecoveryAmount *= RecoveryPackData->RecoveryFactor;
		}
		// @TODO: 회복팩 사용 로직 -> User의 체력 스탯 끌고 오기
		if ( const TScriptInterface<IAbilitySystemInterface>& Interface = User )
		{
			// 체력 Attribute의 값을 증가시켜서 회복시키는 방법
			// GameplayTag를 Heal이랑 Damage에 붙여서 써야할듯
			const FGameplayEffectContextHandle& Handle = Interface->GetAbilitySystemComponent()->MakeEffectContext();
			const FGameplayEffectSpecHandle& SpecHandle = Interface->GetAbilitySystemComponent()->MakeOutgoingSpec
			(
				UNAGE_Heal::StaticClass(),
				RecoveryAmount, // todo: 메디팩 퀄리티에 따른 힐 증가 수치 배수 설정
				Handle
			);
			SpecHandle.Data->SetSetByCallerMagnitude
			(
				FGameplayTag::RequestGameplayTag( TEXT( "Data.Heal" ) ),
				RecoveryPackData->RecoveryAmount
			);
			const FActiveGameplayEffectHandle& EventResult = Interface->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf( *SpecHandle.Data.Get() );
			EventResult.WasSuccessfullyApplied() ? UsedAmount = 1 : UsedAmount = 0;
			return EventResult.WasSuccessfullyApplied(); // 성공 했냐? 안했냐?
			
			// 데미지 효과를 제거해서 힐을 하는 방법이 이거고
			// 현실적으로 데미지와 힐이 등가교환이 되는 경우는 거의 드물기에...
			// FGameplayEffectQuery Query;
			// Query.EffectDefinition = UNAGE_Damage::StaticClass();
			// const int32 Removed = Interface->GetAbilitySystemComponent()->RemoveActiveEffects( Query, 1 );
		}
	}
	return false;
}

EMedPackGrade ANAMedPack::GetMedPackGrade() const
{
	if (GetItemData())
	{
		if (const FNARecoveryPackTableRow* RecoveryDataStructs = GetItemData()->GetItemMetaDataStruct<FNARecoveryPackTableRow>())
		{
			return RecoveryDataStructs->MedPackGrade;
		}
	}

	return EMedPackGrade::MPG_None;
}

// void ANAMedPack::BeginInteract_Implementation(AActor* Interactor)
// {
// 	Super::BeginInteract_Implementation(Interactor);
// }

// bool ANAMedPack::ExecuteInteract_Implementation(AActor* Interactor)
// {
// 	return Super::ExecuteInteract_Implementation(Interactor);
// }

// void ANAMedPack::EndInteract_Implementation(AActor* Interactor)
// {
// 	Super::EndInteract_Implementation(Interactor);
// }


