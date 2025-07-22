#pragma once
#include "KismetNodes/SGraphNodeK2Base.h"

class ARPGEDITOR_API SNAHiddenGraphNodeK2 : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SNAHiddenGraphNodeK2)
	{
		_Visibility = EVisibility::Collapsed;
	}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, UK2Node* InNode );
	
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
