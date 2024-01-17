// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Capstone : ModuleRules
{
	public Capstone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
