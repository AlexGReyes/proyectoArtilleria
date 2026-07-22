using UnrealBuildTool;

public class Artillery : ModuleRules
{
	public Artillery(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SISACore",
			"Ballistics",
			"Ammunition"
		});

		// SISAGIS is only needed by the piece Actor's implementation (geographic
		// placement), so it stays private: consumers of Artillery's public headers
		// never touch the GIS seam through us.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"SISAGIS"
		});
	}
}
