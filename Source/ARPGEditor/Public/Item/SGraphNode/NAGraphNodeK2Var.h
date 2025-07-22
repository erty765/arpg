#pragma once
#include "KismetNodes/SGraphNodeK2Var.h"

class ARPGEDITOR_API SNAGraphNodeK2Var : public SGraphNodeK2Var
{
public:
	SLATE_BEGIN_ARGS(SNAGraphNodeK2Var)
	{
		_Visibility = EVisibility::Collapsed;
	}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, UK2Node* InNode );
	
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
