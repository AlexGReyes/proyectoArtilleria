using UnrealBuildTool;

public class SISAGIS : ModuleRules
{
	public SISAGIS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SISACore",
			"CesiumRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
