#pragma once

#include "EdGraphUtilities.h"
#include "KismetNodes/SGraphNodeK2Var.h"

class SNAGraphNodeK2Var;

struct FNAHideableGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual ~FNAHideableGraphPanelNodeFactory() override {}
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override;
};
