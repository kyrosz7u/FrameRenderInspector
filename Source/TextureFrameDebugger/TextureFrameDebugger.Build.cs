using UnrealBuildTool;
using System.IO;

public class TextureFrameDebugger : ModuleRules
{
	public TextureFrameDebugger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"ApplicationCore",
				"EditorStyle",
				"UnrealEd",
				"Projects",
				"RenderCore",
				"RHI",
				"Renderer"
			}
		);

		// Add Renderer and RenderCore private include paths
		string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.AddRange(new string[] {
			Path.Combine(EnginePath, "Source/Runtime/Renderer/Private"),
			Path.Combine(EnginePath, "Source/Runtime/RenderCore/Private"),
		});
	}
}
