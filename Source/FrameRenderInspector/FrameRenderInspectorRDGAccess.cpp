#include "FrameRenderInspectorRDGAccess.h"
#include "RenderGraphBuilder.h"

void EnumerateRDGBuffers(FRDGBuilder& GraphBuilder, TFunctionRef<void(FRDGBuffer* Buffer)> Callback)
{
	TSet<FRDGBuffer*> SeenBuffers;

	for (const auto& Pair : GraphBuilder.GetPooledBufferOwnershipMap())
	{
		if (Pair.Value && !SeenBuffers.Contains(Pair.Value))
		{
			SeenBuffers.Add(Pair.Value);
			Callback(Pair.Value);
		}
	}

	for (const auto& Pair : GraphBuilder.GetExternalBuffers())
	{
		if (Pair.Value && !SeenBuffers.Contains(Pair.Value))
		{
			SeenBuffers.Add(Pair.Value);
			Callback(Pair.Value);
		}
	}
}
