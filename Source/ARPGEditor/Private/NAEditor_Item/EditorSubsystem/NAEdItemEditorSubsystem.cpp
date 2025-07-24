// Fill out your copyright notice in the Description page of Project Settings.


#include "NAEditor_Item/EditorSubsystem/NAEdItemEditorSubsystem.h"
#include "NAEditor_Item/ItemEditorBridge/NAEdItemBridgeInterface.h"

#include "NAEditor_Misc/NAEdLogCategory.h"

#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"


void UNAEdItemEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogInit, Log, TEXT("%hs"), __FUNCTION__ );
	
	Super::Initialize(Collection);
	
	CachedItemEditorBridge = FNAEdItemBridgeRegistry::Get();
	check(CachedItemEditorBridge);

	if (!CachedItemEditorBridge->IsSoftItemMetaDataInitialized())
	{
		return;
	}
	
	// 3) 메타데이터 인스턴스 빌드
	UE_LOG(NAEdItem, Display, TEXT("[%hs] 아이템 메타데이터 인스턴싱 진행"), __FUNCTION__);
	
	CachedItemEditorBridge->GetItemMetaData().Reserve(
		CachedItemEditorBridge->GetSoftItemMetaData().Num());
	for (const auto& Pair :CachedItemEditorBridge->GetSoftItemMetaData())
	{
		UClass* NewItemClass = Pair.Key.LoadSynchronous();
		check(CachedItemEditorBridge->IsItemActor(NewItemClass));
		
		CachedItemEditorBridge->BroadcastItemClassRegisteredToMetaData(NewItemClass);
		
		// 블루프린트 CDO 동적 초기화 후 재컴파일 -> 동적 초기화한 내용을 블프 에디터 패널에 반영하기 위함
		if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(NewItemClass)))
		{
			FKismetEditorUtilities::CompileBlueprint(
				BP,
				EBlueprintCompileOptions::SkipSave
				| EBlueprintCompileOptions::SkipGarbageCollection
				| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing);
		}
		if (NewItemClass && !Pair.Value.IsNull())
		{
			CachedItemEditorBridge->GetItemMetaData().Emplace(NewItemClass, Pair.Value);
		}
	}

	if (CachedItemEditorBridge->IsSoftItemMetaDataInitialized()
		&& CachedItemEditorBridge->GetSoftItemMetaData().Num() == CachedItemEditorBridge->GetItemMetaData().Num())
	{
		CachedItemEditorBridge->SetItemMetaDataInitialized(true);
		UE_LOG(NAEdItem, Display, TEXT("[%hs] 아이템 메타데이터 인스턴싱 완료"), __FUNCTION__);
	}
	
	// 에디터가 완전히 켜진 뒤에 한 번만 호출
	FCoreDelegates::OnPostEngineInit.AddUObject(this, &UNAEdItemEditorSubsystem::HandlePostEngineInit);
}

void UNAEdItemEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
	CachedItemEditorBridge = nullptr;
}

void UNAEdItemEditorSubsystem::HandlePostEngineInit()
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
