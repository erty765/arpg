#include "NAEditor/NAEdGraphUtilities.h"

#include "NAEditor/BlueprintGraphNode/NAHideableGraphNodeInterface.h"
#include "NAEditor/SGraphNode/SNAHiddenGraphNodeK2.h"

TSharedPtr<SGraphNode> FNAHideableGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (INAHideableGraphNodeInterface* HideableNode = Cast<INAHideableGraphNodeInterface>(Node))
	{
		if (HideableNode->IsHiddenFromEditor())
		{
			if (UK2Node* Derived = Cast<UK2Node>(Node))
			{
				return SNew(SNAHiddenGraphNodeK2, Derived);
			}
		}
	}
		
	return FGraphPanelNodeFactory::CreateNode(Node);
}
