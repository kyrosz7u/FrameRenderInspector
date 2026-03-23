using UnrealBuildTool;

public class FrameRenderInspectorPixelPicker : ModuleRules
{
	public FrameRenderInspectorPixelPicker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd"
			}
		);
	}
}
