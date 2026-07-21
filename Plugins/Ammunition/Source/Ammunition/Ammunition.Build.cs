using UnrealBuildTool;

public class Ammunition : ModuleRules
{
	public Ammunition(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SISACore",
			"Ballistics"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
