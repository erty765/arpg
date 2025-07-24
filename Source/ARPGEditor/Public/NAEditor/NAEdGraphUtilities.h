#pragma once

#include "EdGraphUtilities.h"

class SNAEdHiddenGraphNodeK2;

struct FNAEdHideableGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual ~FNAEdHideableGraphPanelNodeFactory() override {}
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
