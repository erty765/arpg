// Fill out your copyright notice in the Description page of Project Settings.


#include "NAEditor_Item/ItemActorEditor/NAEdItemActorEditorUtils.h"

#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Serialization/AsyncPackageLoader.h"

#include "NAEditor/BlueprintGraphNode/NAEdHideableGraphNode_VariableGet.h"
#include "BlueprintVariableNodeSpawner.h"
#include "Engine/SCS_Node.h"

bool FNAEdItemActorEditorUtils::EnsureForceNonDataOnlyVariableAdded(UBlueprint* BP, FProperty* VariableProp)
{
	check(IsValid(BP));
	check(VariableProp != nullptr);
	
	if (BP->bRunConstructionScriptOnDrag)
	{
		BP->bRunConstructionScriptOnDrag = false;
	}
	
	const FString& PropName = VariableProp->GetNameCPP();
	
	TArray<UEdGraph*> Graphs;
	BP->GetAllGraphs(Graphs);
	if (Graphs.IsEmpty()) return false;
	
	bool Result = false;
	
	for (UEdGraph* Graph : Graphs)
	{
		if (!Graph->GetName().Equals(TEXT("EventGraph"))) continue;

		bool bShouldAddDummyNode = true;
		TArray<UEdGraphNode*> GNodes = Graph->Nodes;
		if (GNodes.Num() > 0)
		{
			for (UEdGraphNode* GNode : GNodes)
			{
				if (UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(GNode))
				{
					bShouldAddDummyNode
						= !VarNode->GetVarNameString().Equals(PropName);
					if (!bShouldAddDummyNode) break;
				}
			}
		}
		if (bShouldAddDummyNode)
		{
			UBlueprintVariableNodeSpawner* GetterSpawner
				= UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(
					UNAEdHideableGraphNode_VariableGet::StaticClass()
					, VariableProp);
			check(GetterSpawner != nullptr);
			UEdGraphNode* NewGetterNode = GetterSpawner->Invoke(
				Graph
				, IBlueprintNodeBinder::FBindingSet()
				, FVector2D(0.f, -10.f));
			if (INAEdHideableGraphNodeInterface* HideableGetterNode
				= Cast<INAEdHideableGraphNodeInterface>(NewGetterNode))
			{
				HideableGetterNode->SetHiddenFromEditor(true);
			}
			Result = NewGetterNode ? true : false;
		}
	}
	
	if (Result)
	{
		BP->MarkPackageDirty();
	}
	
	return Result;
}

void FNAEdItemActorEditorUtils::UpdateSCSParentComponentReference(UBlueprintGeneratedClass* BPGC,
	const FString& MatchPrefix, const FName& NewParentName)
{
	check(BPGC);
	
	TArray<USCS_Node*> SCSNodes = BPGC->SimpleConstructionScript->GetAllNodes();
	if (SCSNodes.IsEmpty()) return;

	for (USCS_Node* SCSNode : SCSNodes)
	{
		FName ParentComponentName = SCSNode->ParentComponentOrVariableName;
		if (ParentComponentName.ToString().StartsWith(MatchPrefix)
			&& !ParentComponentName.IsEqual(NewParentName))
		{
			SCSNode->ParentComponentOrVariableName = NewParentName;
			return;
		}
	}
}

void FNAEdItemActorEditorUtils::CompileBlueprintWithOptionalStructuralMark(UBlueprint* BP,
	bool bMarkStructurallyModified)
{
	check(BP);
	
	if (bMarkStructurallyModified)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	}
	
	FKismetEditorUtilities::CompileBlueprint(
		BP,
		EBlueprintCompileOptions::SkipSave
		| EBlueprintCompileOptions::SkipGarbageCollection
		| EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing);
}

// ---------------------------------------------------------------------------
// 지연 컴파일 큐
// ---------------------------------------------------------------------------
namespace
{
	// 키: 컴파일 대상 BP, 값: 구조 변경 마크 여부 (중복 요청은 OR로 합침)
	TMap<TWeakObjectPtr<UBlueprint>, bool> GPendingDeferredCompiles;
	FTSTicker::FDelegateHandle GDeferredCompileTickHandle;
}

void FNAEdItemActorEditorUtils::RequestDeferredCompile(UBlueprint* BP, bool bMarkStructurallyModified)
{
	check(BP);

	bool& bPendingMark = GPendingDeferredCompiles.FindOrAdd(BP, false);
	bPendingMark |= bMarkStructurallyModified;

	if (!GDeferredCompileTickHandle.IsValid())
	{
		GDeferredCompileTickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateStatic(&FNAEdItemActorEditorUtils::FlushDeferredCompiles), 0.0f);
	}
}

bool FNAEdItemActorEditorUtils::FlushDeferredCompiles(float /*DeltaTime*/)
{
	// 로드/리인스턴싱/저장/PIE 중에는 컴파일하지 않고 다음 틱으로 미룸 (true = 티커 유지)
	const bool bUnsafeNow =
		GIsEditorLoadingPackage
		|| IsAsyncLoading()
		|| GIsReconstructingBlueprintInstances
		|| GIsSavingPackage
		|| GEditor == nullptr
		|| GEditor->PlayWorld != nullptr;
	if (bUnsafeNow)
	{
		return true;
	}

	TMap<TWeakObjectPtr<UBlueprint>, bool> Pending = MoveTemp(GPendingDeferredCompiles);
	GPendingDeferredCompiles.Empty();
	GDeferredCompileTickHandle.Reset();

	for (const TPair<TWeakObjectPtr<UBlueprint>, bool>& Pair : Pending)
	{
		UBlueprint* BP = Pair.Key.Get();
		if (!IsValid(BP) || BP->bBeingCompiled) continue;

		CompileBlueprintWithOptionalStructuralMark(BP, Pair.Value);
	}

	// 컴파일 도중 새 요청이 들어왔다면 RequestDeferredCompile이 티커를 다시 등록했으므로 이 인스턴스는 종료
	return false;
}

void FNAEdItemActorEditorUtils::ShutdownDeferredCompiles()
{
	if (GDeferredCompileTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(GDeferredCompileTickHandle);
		GDeferredCompileTickHandle.Reset();
	}
	GPendingDeferredCompiles.Empty();
}
