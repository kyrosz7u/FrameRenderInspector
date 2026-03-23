using UnrealBuildTool;

public class FrameRenderInspectorPixelPicker : ModuleRules
{
	public FrameRenderInspectorPixelPicker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
