// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ARPGEditor : ModuleRules
{
	public ARPGEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"GraphEditor",
			"BlueprintGraph",
			"UnrealEd",
			"KismetCompiler",
			"BlueprintGraph",
			"EditorSubsystem"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ARPG",
		});
	}
}
