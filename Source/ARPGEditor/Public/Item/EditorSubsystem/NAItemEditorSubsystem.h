// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "NAItemEditorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN( LogNAItemEditor, Log, All );

/**
 * 
 */
UCLASS()
class ARPGEDITOR_API UNAItemEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void HandlePostEngineInit();
};
