// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ContactHostile : ModuleRules
{
	public ContactHostile(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });

		PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
