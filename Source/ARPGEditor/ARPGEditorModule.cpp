#include "ARPGEditorModule.h"

#include "NAEdGraphUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"

IMPLEMENT_MODULE(FARPGEditorModule, ARPGEditor);

void FARPGEditorModule::StartupModule()
{
	NAHideableGraphNodeFactory = MakeShareable(new FNAHideableGraphPanelNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(NAHideableGraphNodeFactory);
}

void FARPGEditorModule::ShutdownModule()
{
	FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);
	
	FEdGraphUtilities::UnregisterVisualNodeFactory(NAHideableGraphNodeFactory);
	NAHideableGraphNodeFactory.Reset();
}
