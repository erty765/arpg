// Copyright Epic Games, Inc. All Rights Reserved.

#include "ARPG.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "NAPlayerController.h"
#include "HP/GameplayEffect/NAGE_Damage.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FARPGModule, ARPG, "ARPG" );

static FAutoConsoleCommandWithWorldAndArgs SmiteCommand(
	TEXT("smite"),
	TEXT("Give the damage to the given player controller"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() == 2)
		{
			const FString Param = Args[0];
			const FString Param2 = Args[1];
			
			const int32 TargetIndex = FCString::Atoi(*Param);
			int32 Index = 0;
			float Damage = FCString::Atof(*Param2);

			ANAPlayerController* TargetPlayerController = nullptr;
			
			for ( auto It = World->GetPlayerControllerIterator(); It; ++It )
			{
				if ( Index == TargetIndex )
				{
					TargetPlayerController = Cast<ANAPlayerController>( It->Get() );
					break;
				}
				++Index;
			}

			if ( TargetPlayerController )
			{
				if ( const TScriptInterface<IAbilitySystemInterface>& Interface = TargetPlayerController->GetPawn() )
				{
					const FGameplayEffectContextHandle Context = Interface->GetAbilitySystemComponent()->MakeEffectContext();
					const FGameplayEffectSpecHandle Spec = Interface->GetAbilitySystemComponent()->MakeOutgoingSpec( UNAGE_Damage::StaticClass(), 1.f, Context );
					Spec.Data->SetSetByCallerMagnitude( FGameplayTag::RequestGameplayTag( "Data.Damage" ), Damage );
					Interface->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf( *Spec.Data.Get() );
				}
			}
		}
	})
);

void ARPGModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();

#if WITH_EDITOR
	NAHideableGraphNodeFactory = MakeShareable(new FNAHideableGraphPanelNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(NAHideableGraphNodeFactory);
#endif
}
