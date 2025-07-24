#include "ARPGEditorModule.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Public/NAEditor/NAEdGraphUtilities.h"

IMPLEMENT_MODULE(FARPGEditorModule, ARPGEditor);

void FARPGEditorModule::StartupModule()
{
	NAHideableGraphNodeFactory = MakeShareable(new FNAEdHideableGraphPanelNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(NAHideableGraphNodeFactory);
}

void FARPGEditorModule::ShutdownModule()
{
	FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);
	
	FEdGraphUtilities::UnregisterVisualNodeFactory(NAHideableGraphNodeFactory);
	NAHideableGraphNodeFactory.Reset();
}
