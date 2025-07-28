// Copyright Epic Games, Inc. All Rights Reserved.

using System.Diagnostics;
using System.IO;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildTool;

public class ARPGEditor : ModuleRules
{
	public ARPGEditor(ReadOnlyTargetRules target) : base(target)
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
		
		if (Target.bBuildEditor)
		{ 
		// GenerateBridgeWrappers 스크립트 추가 ///////////////////////////////////////////////////////////////////////////
			
		
			string moduleDir = Path.GetFullPath(ModuleDirectory);
			string scriptPath = Path.Combine(moduleDir, "../../Scripts/GenerateBridgeWrappers.py");
			string fullScriptPath = Path.GetFullPath(scriptPath);
/*
			if (File.Exists(fullScriptPath))
			{
				string pythonExe = "python";
				
				Logger.LogInformation("Running wrapper generator: " + fullScriptPath);
                
				var startInfo = new ProcessStartInfo
				{
					FileName = pythonExe,
					Arguments = $"\"{fullScriptPath}\" \"{moduleDir}\"",
					UseShellExecute = false,
					RedirectStandardOutput = true,
					RedirectStandardError = true,
					CreateNoWindow = true,
                    WorkingDirectory = Path.GetDirectoryName(fullScriptPath)
                };

				using (Process process = Process.Start(startInfo))
				{
					process.OutputDataReceived += (sender, args) =>
					{
						if (!string.IsNullOrEmpty(args.Data)) Logger.LogWarning("[BridgeWrapper] " + args.Data);
					};
					process.ErrorDataReceived += (sender, args) =>
					{
						if (!string.IsNullOrEmpty(args.Data)) Logger.LogWarning("[BridgeWrapper][Error] " + args.Data);
					};
					process.BeginOutputReadLine();
					process.BeginErrorReadLine();
					process.WaitForExit();
				}
			}
			else
			{
				Logger.LogWarning($"[BridgeWrapper] Python script not found at: {fullScriptPath}");
			}
			*/
			
		// BridgeWrapper include path 추가 (ProjectRoot/Intermediate/BridgeWrappers/ModuleName/) ////////////////////////
			
			string projectRoot = Path.GetFullPath(Path.Combine(moduleDir, "..", ".."));
			string moduleName = Path.GetFileName(moduleDir);
			
			string wrapperIncludePath = Path.Combine(
				projectRoot,
				"Intermediate",
				"BridgeWrappers",
				moduleName);

			PublicIncludePaths.Add(wrapperIncludePath);
			Logger.LogInformation("[BridgeWrapper] IncludePath 추가: " + wrapperIncludePath);
		}
	}
}
