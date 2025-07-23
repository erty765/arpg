#include "NAEditor/SGraphNode/SNAHiddenGraphNodeK2.h"

void SNAHiddenGraphNodeK2::Construct(const FArguments& InArgs, UK2Node* InNode)
{
	GraphNode = InNode;
}

int32 SNAHiddenGraphNodeK2::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                                 const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return 0;
}
