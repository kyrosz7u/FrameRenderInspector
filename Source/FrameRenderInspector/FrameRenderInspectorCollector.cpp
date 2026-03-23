#include "FrameRenderInspectorCollector.h"
#include "FrameRenderInspectorModule.h"
#include "FrameRenderInspectorRDGAccess.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "RHIGPUReadback.h"
#include "Async/Async.h"
#include "Misc/ScopeLock.h"

extern RENDERER_API FRenderTargetPool GRenderTargetPool;

class FFrameRenderInspectorOverlayPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFrameRenderInspectorOverlayPS);
	SHADER_USE_PARAMETER_STRUCT(FFrameRenderInspectorOverlayPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(float, OverlayOpacity)
		SHADER_PARAMETER(float, NormalizeMin)
		SHADER_PARAMETER(float, NormalizeInvRange)
		SHADER_PARAMETER(uint32, ApplyRangeNormalization)
		SHADER_PARAMETER(uint32, VisualizeAsSingleChannel)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FFrameRenderInspectorOverlayPS, "/Plugin/FrameRenderInspector/Private/FrameRenderInspectorOverlay.usf", "MainPS", SF_Pixel);

class FFrameRenderInspectorComputeVisibleRangeCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFrameRenderInspectorComputeVisibleRangeCS);
	SHADER_USE_PARAMETER_STRUCT(FFrameRenderInspectorComputeVisibleRangeCS, FGlobalShader);

public:
	static constexpr int32 ThreadGroupSizeX = 8;
	static constexpr int32 ThreadGroupSizeY = 8;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER(FIntPoint, InputRectMin)
		SHADER_PARAMETER(FIntPoint, InputRectMax)
		SHADER_PARAMETER(uint32, ComputeVisualizeAsSingleChannel)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, RWVisibleRangeBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FFrameRenderInspectorComputeVisibleRangeCS, "/Plugin/FrameRenderInspector/Private/FrameRenderInspectorOverlay.usf", "ComputeVisibleRangeCS", SF_Compute);

class FFrameRenderInspectorSamplePixelCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFrameRenderInspectorSamplePixelCS);
	SHADER_USE_PARAMETER_STRUCT(FFrameRenderInspectorSamplePixelCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER(FIntPoint, SourcePixelCoord)
		SHADER_PARAMETER(float, NormalizeMin)
		SHADER_PARAMETER(float, NormalizeInvRange)
		SHADER_PARAMETER(uint32, ApplyRangeNormalization)
		SHADER_PARAMETER(uint32, VisualizeAsSingleChannel)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, RWSampledPixelBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FFrameRenderInspectorSamplePixelCS, "/Plugin/FrameRenderInspector/Private/FrameRenderInspectorOverlay.usf", "SamplePixelCS", SF_Compute);

static uint32 EncodeOrderedFloat(float Value)
{
	union
	{
		float FloatValue;
		uint32 UintValue;
	} Converter;

	Converter.FloatValue = Value;
	return Converter.UintValue ^ ((Converter.UintValue & 0x80000000u) ? 0xFFFFFFFFu : 0x80000000u);
}

static float DecodeOrderedFloat(uint32 Value)
{
	union
	{
		float FloatValue;
		uint32 UintValue;
	} Converter;

	Converter.UintValue = Value ^ ((Value & 0x80000000u) ? 0x80000000u : 0xFFFFFFFFu);
	return Converter.FloatValue;
}

static float DecodeRawFloat(uint32 Value)
{
	union
	{
		float FloatValue;
		uint32 UintValue;
	} Converter;

	Converter.UintValue = Value;
	return Converter.FloatValue;
}

static void AddFrameRenderInspectorOverlayPass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef InputTexture,
	FIntRect InputRect,
	FRDGTextureRef OutputTexture,
	FIntRect OutputRect,
	float OverlayOpacity,
	bool bApplyRangeNormalization,
	bool bVisualizeAsSingleChannel,
	float NormalizeMin,
	float NormalizeMax)
{
	const FScreenPassTextureViewport InputViewport(InputTexture, InputRect);
	const FScreenPassTextureViewport OutputViewport(OutputTexture, OutputRect);

	TShaderMapRef<FScreenPassVS> VertexShader(View.ShaderMap);
	TShaderMapRef<FFrameRenderInspectorOverlayPS> PixelShader(View.ShaderMap);

	FFrameRenderInspectorOverlayPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFrameRenderInspectorOverlayPS::FParameters>();
	PassParameters->Output = GetScreenPassTextureViewportParameters(OutputViewport);
	PassParameters->InputTexture = InputTexture;
	PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->OverlayOpacity = OverlayOpacity;
	PassParameters->NormalizeMin = NormalizeMin;
	PassParameters->NormalizeInvRange = (NormalizeMax > NormalizeMin) ? (1.0f / (NormalizeMax - NormalizeMin)) : 1.0f;
	PassParameters->ApplyRangeNormalization = bApplyRangeNormalization ? 1u : 0u;
	PassParameters->VisualizeAsSingleChannel = bVisualizeAsSingleChannel ? 1u : 0u;
	PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);

	FRHIBlendState* BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI();
	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("FrameRenderInspectorOverlay"),
		View,
		OutputViewport,
		InputViewport,
		VertexShader,
		PixelShader,
		BlendState,
		PassParameters);
}

static void AddComputeVisibleRangePass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef InputTexture,
	FIntRect InputRect,
	bool bVisualizeAsSingleChannel,
	FRDGBufferRef VisibleRangeBuffer)
{
	TShaderMapRef<FFrameRenderInspectorComputeVisibleRangeCS> ComputeShader(View.ShaderMap);

	FFrameRenderInspectorComputeVisibleRangeCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFrameRenderInspectorComputeVisibleRangeCS::FParameters>();
	PassParameters->InputTexture = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputTexture));
	PassParameters->InputRectMin = InputRect.Min;
	PassParameters->InputRectMax = InputRect.Max;
	PassParameters->ComputeVisualizeAsSingleChannel = bVisualizeAsSingleChannel ? 1u : 0u;
	PassParameters->RWVisibleRangeBuffer = GraphBuilder.CreateUAV(VisibleRangeBuffer);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("TextureFrameComputeVisibleRange"),
		ComputeShader,
		PassParameters,
		FComputeShaderUtils::GetGroupCount(InputRect.Size(), FIntPoint(FFrameRenderInspectorComputeVisibleRangeCS::ThreadGroupSizeX, FFrameRenderInspectorComputeVisibleRangeCS::ThreadGroupSizeY)));
}

static void AddSamplePixelPass(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef InputTexture,
	FIntPoint SourcePixelCoord,
	bool bApplyRangeNormalization,
	bool bVisualizeAsSingleChannel,
	float NormalizeMin,
	float NormalizeMax,
	FRDGBufferRef SampledPixelBuffer)
{
	TShaderMapRef<FFrameRenderInspectorSamplePixelCS> ComputeShader(View.ShaderMap);

	FFrameRenderInspectorSamplePixelCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FFrameRenderInspectorSamplePixelCS::FParameters>();
	PassParameters->InputTexture = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputTexture));
	PassParameters->SourcePixelCoord = SourcePixelCoord;
	PassParameters->NormalizeMin = NormalizeMin;
	PassParameters->NormalizeInvRange = (NormalizeMax > NormalizeMin) ? (1.0f / (NormalizeMax - NormalizeMin)) : 1.0f;
	PassParameters->ApplyRangeNormalization = bApplyRangeNormalization ? 1u : 0u;
	PassParameters->VisualizeAsSingleChannel = bVisualizeAsSingleChannel ? 1u : 0u;
	PassParameters->RWSampledPixelBuffer = GraphBuilder.CreateUAV(SampledPixelBuffer);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("TextureFrameSamplePixel"),
		ComputeShader,
		PassParameters,
		FIntVector(1, 1, 1));
}

#include "Collector/FrameRenderInspectorCollectorState.inl"
#include "Collector/FrameRenderInspectorCollectorReadback.inl"
#include "Collector/FrameRenderInspectorCollectorRendering.inl"
