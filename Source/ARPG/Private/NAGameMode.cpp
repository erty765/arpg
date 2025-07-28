// Copyright Epic Games, Inc. All Rights Reserved.

#include "NAGameMode.h"

#include "EngineUtils.h"
#include "NACharacter.h"
#include "NAGameStateBase.h"
#include "NAInGameHUD.h"
#include "NAPlayerController.h"
#include "NAPlayerState.h"
#include "Assets/Interface/NAManagedAsset.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ANAGameMode::ANAGameMode()
{
	DefaultPawnClass = ANACharacter::StaticClass();
	PlayerStateClass = ANAPlayerState::StaticClass();
	GameStateClass = ANAGameStateBase::StaticClass();
	PlayerControllerClass = ANAPlayerController::StaticClass();
	HUDClass = ANAInGameHUD::StaticClass();
	bUseSeamlessTravel = true;
}

APawn* ANAGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	SpawnInfo.CustomPreSpawnInitalization = [NewPlayer](AActor* InActor)
	{
		// 기존의 SpawnDefaultPawnAtTransform 구현부 복사 붙여넣기 + PreSpawn 함수 추가
		// PlayerState가 먼저 만들어지고 (캐릭터 에셋이 부여되고), 그 다음에 Pawn이 생성됨
		if (const ANAPlayerController* PlayerController = Cast<ANAPlayerController>(NewPlayer))
		{
			if (const ANAPlayerState* PlayerState = PlayerController->GetPlayerState<ANAPlayerState>())
			{
				if (const TScriptInterface<INAManagedAsset> Interface = InActor)
				{
					Interface->SetAssetName(PlayerState->GetPossessAssetName());
				}
			}
		}
	};
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	if (!ResultPawn)
	{
		UE_LOG(LogGameMode, Warning, TEXT("SpawnDefaultPawnAtTransform: Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
	}
	
	return ResultPawn;
}

AActor* ANAGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), FoundActors);

	ANAPlayerState* PS = Cast<ANAPlayerState>(Player->PlayerState);
	if (!PS) return Super::ChoosePlayerStart_Implementation(Player);

	PS->PlayerNumber = PlayerCount++;
	
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(*It);
		if (PlayerStart->PlayerStartTag == FName(*FString::Printf(TEXT("Player0%i"),PS->PlayerNumber)))
		{
			return PlayerStart;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
	
	
	// temp 에디터 시작시 player01로 고정
	// if (FoundActors.Num() == 0)
	// 	return Super::ChoosePlayerStart_Implementation(Player);
	//
	// int32 Index = 0;
	//
	// // if (Player && Player->PlayerState)
	// // 	Index = Player->PlayerState->GetPlayerId();
	//
	// // int32 ChosenIndex = FMath::Clamp(Index, 0, FoundActors.Num() - 1);
	// return FoundActors[Index];
}
