#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NAHideableGraphNodeInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UNAHideableGraphNodeInterface : public UInterface
{
	GENERATED_BODY()
};

class ARPGEDITOR_API INAHideableGraphNodeInterface
{
	GENERATED_BODY()

public:
	virtual bool IsHiddenFromEditor() const = 0;
	virtual void SetHiddenFromEditor(const bool bHide) = 0;
};
