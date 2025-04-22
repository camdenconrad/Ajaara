using UnrealBuildTool;

public class MyProjectTest2 : ModuleRules
{
	public MyProjectTest2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"HeadMountedDisplay", 
			"EnhancedInput",
			"UMG",
			"Niagara" // Add this line to include the Niagara module
		});
	}
}