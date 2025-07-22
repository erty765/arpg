#include "Item/SGraphNode/NAGraphNodeK2Var.h"

void SNAGraphNodeK2Var::Construct(const FArguments& InArgs, UK2Node* InNode)
{
	GraphNode = InNode;
}

int32 SNAGraphNodeK2Var::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                                 const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return 0;
}
