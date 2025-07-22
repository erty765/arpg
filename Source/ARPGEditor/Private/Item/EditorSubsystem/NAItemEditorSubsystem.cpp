// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EditorSubsystem/NAItemEditorSubsystem.h"

#include "FileHelpers.h"
#include "Item/EngineSubsystem/NAItemEngineSubsystem.h"
#include "Item/ItemActor/NAItemActor.h"
#include "Kismet2/KismetEditorUtilities.h"

DEFINE_LOG_CATEGORY( LogNAItemEditor )

void UNAItemEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG( LogInit, Log, TEXT("%hs"), __FUNCTION__ )
	
	// 4) 메타데이터 맵 빌드
	UE_LOG(LogNAItemEditor, Display, TEXT("[%hs] 아이템 메타데이터 매핑 진행 (Editor)"), __FUNCTION__);

	const UNAItemEngineSubsystem* Subsystem = UNAItemEngineSubsystem::Get();
	check( Subsystem && Subsystem->bMetaDataInitialized );
	
	if ( Subsystem )
	{
		for (const auto& Pair : Subsystem->SoftItemMetaData)
		{
			UClass* NewItemActorClass = Pair.Key.LoadSynchronous();
			if (ANAItemActor* ItemActorCDO = Cast<ANAItemActor>(
				NewItemActorClass->GetDefaultObject(false)))
			{
				ItemActorCDO->OnItemClassRegisteredToMetaData.ExecuteIfBound();
			}
			// 블루프린트 CDO 동적 초기화 후 재컴파일 -> 동적 초기화한 내용을 블프 에디터 패널에 반영하기 위함
			if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(NewItemActorClass)))
			{
				FKismetEditorUtilities::CompileBlueprint(
					BP,
					EBlueprintCompileOptions::SkipSave
					| EBlueprintCompileOptions::SkipGarbageCollection
					| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing);
			}
		}
	}

	// 에디터가 완전히 켜진 뒤에 한 번만 호출
	FCoreDelegates::OnPostEngineInit.AddUObject(this, &UNAItemEditorSubsystem::HandlePostEngineInit);
}

void UNAItemEditorSubsystem::HandlePostEngineInit()
{
	// 한 번만 실행되도록 바인딩 해제
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// 더티된 모든 패키지를 저장
	FEditorFileUtils::SaveDirtyPackages(
		/*bPromptUserToSave=*/		false,
		/*bSaveMapPackages=*/		true,
		/*bSaveContentPackages=*/	true,
		/*bFastSave=*/				true);
}
