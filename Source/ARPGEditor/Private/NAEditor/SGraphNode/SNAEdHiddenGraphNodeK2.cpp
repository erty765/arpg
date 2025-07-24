#include "NAEditor/SGraphNode/SNAEdHiddenGraphNodeK2.h"

void SNAEdHiddenGraphNodeK2::Construct(const FArguments& InArgs, UK2Node* InNode)
{
	GraphNode = InNode;
}

int32 SNAEdHiddenGraphNodeK2::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                                 const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return 0;
}
