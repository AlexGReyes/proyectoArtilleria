using UnrealBuildTool;

public class Hardware : ModuleRules
{
	public Hardware(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Deliberately depends on SISACore only: the Phase 2 transports (serial,
		// RS485, CAN, TCP/UDP) will add their own dependencies inside this module,
		// never the other way round.
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
