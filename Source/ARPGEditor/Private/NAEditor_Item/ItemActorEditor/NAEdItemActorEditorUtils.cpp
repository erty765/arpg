// Fill out your copyright notice in the Description page of Project Settings.


#include "NAEditor_Item/ItemActorEditor/NAEdItemActorEditorUtils.h"

#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

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
