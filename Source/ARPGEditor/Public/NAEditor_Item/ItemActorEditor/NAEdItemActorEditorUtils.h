// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct ARPGEDITOR_API FNAEdItemActorEditorUtils
{
	static bool EnsureForceNonDataOnlyVariableAdded(UBlueprint* BP, FProperty* VariableProp);
	
	static void UpdateSCSParentComponentReference(UBlueprintGeneratedClass* BPGC
		, const FString& MatchPrefix, const FName& NewParentName);
	
	static void CompileBlueprintWithOptionalStructuralMark(UBlueprint* BP, bool bMarkStructurallyModified = false);

	// 현재 콜스택 밖(다음 에디터 틱)에서 BP를 1회 컴파일. 같은 BP의 중복 요청은 합쳐짐.
	// OnConstruction/로드/리인스턴싱 스택 위에서 동기 컴파일하면 자기 클래스가 REINST_로 바뀌므로 반드시 이 경로를 쓸 것.
	static void RequestDeferredCompile(UBlueprint* BP, bool bMarkStructurallyModified = true);

	// 대기 중인 지연 컴파일 요청 폐기 + 티커 해제 (에디터 서브시스템 Deinitialize에서 호출)
	static void ShutdownDeferredCompiles();

private:
	static bool FlushDeferredCompiles(float DeltaTime);
};
