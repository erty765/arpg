// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct ARPGEDITOR_API FNAEdItemActorEditorUtils
{
	static bool EnsureForceNonDataOnlyVariableAdded(UBlueprint* BP, FProperty* VariableProp);
	
	static void UpdateSCSParentComponentReference(UBlueprintGeneratedClass* BPGC
		, const FString& MatchPrefix, const FName& NewParentName);
	
	static void CompileBlueprintWithOptionalStructuralMark(UBlueprint* BP, bool bMarkStructurallyModified = false);
};
