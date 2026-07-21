using UnrealBuildTool;

public class Ballistics : ModuleRules
{
	public Ballistics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SISACore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
