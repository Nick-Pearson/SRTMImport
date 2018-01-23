// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SRTMImport : ModuleRules
	{
		public SRTMImport(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            
			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
                "CoreUObject",
                "Engine",
            });

            PrivateDependencyModuleNames.AddRange(new string[] {
            });
        }
	}
}
