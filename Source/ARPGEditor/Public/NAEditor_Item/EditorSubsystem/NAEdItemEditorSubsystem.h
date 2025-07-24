// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "NAEdItemEditorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(NAEdItem, Log, All);

/**
 * 
 */
UCLASS()
class ARPGEDITOR_API UNAEdItemEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UNAEdItemEditorSubsystem* Get()
	{
		if (GEditor)
		{
			return GEditor->GetEditorSubsystem<UNAEdItemEditorSubsystem>();
		}
		return nullptr;
	}
	
protected:
	void HandlePostEngineInit();

private:
	class INAEdItemBridge* CachedItemEditorBridge = nullptr;
};


