// Fill out your copyright notice in the Description page of Project Settings.


#include "NAEditor_Item/EditorSubsystem/NAEdItemEditorSubsystem.h"
#include "NAEditor_Item/ItemEditorBridge/NAEdItemBridge.h"

#include "NAEditor_Misc/NAEdLogCategory.h"

#include "FileHelpers.h"


void UNAEdItemEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogInit, Log, TEXT("%hs"), __FUNCTION__ );
	
	Super::Initialize(Collection);

	if (!FNAEdItemBridge::IsSoftItemMetaDataInitialized())
	{
		return;
	}
	
	// 3) 메타데이터 인스턴스 빌드
	UE_LOG(NAEdItem, Display, TEXT("[%hs] 아이템 메타데이터 인스턴싱 진행"), __FUNCTION__);
	
	FNAEdItemBridge::GetItemMetaData().Reserve(FNAEdItemBridge::GetSoftItemMetaData().Num());
	for (const auto& Pair : FNAEdItemBridge::GetSoftItemMetaData())
	{
		UClass* NewItemClass = Pair.Key.LoadSynchronous();
		check(FNAEdItemBridge::IsItemActor(NewItemClass));
		if (NewItemClass && !Pair.Value.IsNull())
		{
			FNAEdItemBridge::GetItemMetaData().Emplace(NewItemClass, Pair.Value);
			FNAEdItemBridge::BroadcastItemClassRegisteredToMetaData(NewItemClass);
		}
	}

	if (FNAEdItemBridge::IsSoftItemMetaDataInitialized()
		&& FNAEdItemBridge::GetSoftItemMetaData().Num() == FNAEdItemBridge::GetItemMetaData().Num())
	{
		FNAEdItemBridge::SetItemMetaDataInitialized(true);
		UE_LOG(NAEdItem, Display, TEXT("[%hs] 아이템 메타데이터 인스턴싱 완료"), __FUNCTION__);
	}
	
	// 에디터가 완전히 켜진 뒤에 한 번만 호출
	FCoreDelegates::OnPostEngineInit.AddUObject(this, &UNAEdItemEditorSubsystem::HandlePostEngineInit);
}

void UNAEdItemEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();
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
