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
			string scriptPath = Path.Combine(ModuleDirectory, "../../Scripts/ScanForBridgeWrappers.py");
			// 절대 경로로 변환
			string fullScriptPath = Path.GetFullPath(scriptPath);

			if (File.Exists(fullScriptPath))
			{
				string pythonExe = "python";
				
				Logger.LogInformation("Running wrapper generator script: " + fullScriptPath);
                
				var startInfo = new ProcessStartInfo
				{
					FileName = pythonExe,
					Arguments = $"\"{fullScriptPath}\"",
					UseShellExecute = false,
					RedirectStandardOutput = true,
					RedirectStandardError = true,
					CreateNoWindow = true
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
		}
	}
}
