#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NAEdHideableGraphNodeInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UNAEdHideableGraphNodeInterface : public UInterface
{
	GENERATED_BODY()
};

class ARPGEDITOR_API INAEdHideableGraphNodeInterface
{
	GENERATED_BODY()

public:
	virtual bool IsHiddenFromEditor() const = 0;
	virtual void SetHiddenFromEditor(const bool bHide) = 0;
};
