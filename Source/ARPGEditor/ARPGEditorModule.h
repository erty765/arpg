#pragma once
#include "Modules/ModuleManager.h"

struct FGraphPanelNodeFactory;

class FARPGEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FGraphPanelNodeFactory> NAHideableGraphNodeFactory;
};
