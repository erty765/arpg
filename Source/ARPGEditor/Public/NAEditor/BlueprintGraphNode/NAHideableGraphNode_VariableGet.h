// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "K2Node_VariableGet.h"
#include "NAHideableGraphNodeInterface.h"
#include "NAHideableGraphNode_VariableGet.generated.h"

/**
 * 
 */
UCLASS()
class ARPGEDITOR_API UNAHideableGraphNode_VariableGet : public UK2Node_VariableGet, public INAHideableGraphNodeInterface
{
	GENERATED_BODY()

//======================================================================================================================
// Hideable Node Interface Implements
//======================================================================================================================
public:
	virtual bool IsHiddenFromEditor() const override
	{
		return bHiddenFromEditor;
	}
	
	virtual void SetHiddenFromEditor(const bool bHide) override
	{
		bHiddenFromEditor = bHide;
	}

protected:
	UPROPERTY()
	uint8 bHiddenFromEditor : 1 = false;
};
