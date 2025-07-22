#include "NAEdGraphUtilities.h"

#include "K2Node_VariableGet.h"
#include "NABlueprintGraphNode/NAHideableGraphNodeInterface.h"
#include "Item/SGraphNode/NAGraphNodeK2Var.h"

TSharedPtr<class SGraphNode> FNAHideableGraphPanelNodeFactory::CreateNode(class UEdGraphNode* Node) const
{
	if (INAHideableGraphNodeInterface* HideableNode = Cast<INAHideableGraphNodeInterface>(Node))
	{
		if (HideableNode->IsHiddenFromEditor())
		{
			if ( UK2Node_Variable* Derived = Cast<UK2Node_Variable>(Node) )
			{
				return SNew(SNAGraphNodeK2Var, Derived);
			}
		}
	}
		
	return FGraphPanelNodeFactory::CreateNode(Node);
}
