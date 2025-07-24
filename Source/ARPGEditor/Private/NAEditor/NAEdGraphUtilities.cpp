#include "NAEditor/NAEdGraphUtilities.h"

#include "NAEditor/BlueprintGraphNode/NAEdHideableGraphNodeInterface.h"
#include "NAEditor/SGraphNode/SNAEdHiddenGraphNodeK2.h"

TSharedPtr<SGraphNode> FNAEdHideableGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (INAEdHideableGraphNodeInterface* HideableNode = Cast<INAEdHideableGraphNodeInterface>(Node))
	{
		if (HideableNode->IsHiddenFromEditor())
		{
			if (UK2Node* Derived = Cast<UK2Node>(Node))
			{
				return SNew(SNAEdHiddenGraphNodeK2, Derived);
			}
		}
	}
		
	return FGraphPanelNodeFactory::CreateNode(Node);
}
