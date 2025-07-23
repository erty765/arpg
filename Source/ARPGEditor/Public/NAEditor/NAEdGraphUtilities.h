#pragma once

#include "EdGraphUtilities.h"

class SNAHiddenGraphNodeK2;

struct FNAHideableGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual ~FNAHideableGraphPanelNodeFactory() override {}
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
