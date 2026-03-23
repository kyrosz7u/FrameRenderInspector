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

FFrameRenderInspectorCollector::FFrameRenderInspectorCollector(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
	, bIsCaptureEnabled(false)
{
	AutoRangeReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FrameRenderInspector.VisibleRangeReadback"));
	BufferReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FrameRenderInspector.BufferReadback"));
}

void FFrameRenderInspectorCollector::SetCaptureEnabled(bool bEnabled)
{
	bIsCaptureEnabled = bEnabled;
}

void FFrameRenderInspectorCollector::SetSelectedTexture(const FString& TextureName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	SelectedTextureName = TextureName;
}

void FFrameRenderInspectorCollector::SetSelectedBuffer(const FString& BufferName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	SelectedBufferName = BufferName;
}

void FFrameRenderInspectorCollector::SetOverlayOpacity(float InOpacity)
{
	FScopeLock Lock(&SelectedTextureMutex);
	OverlayOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
}

void FFrameRenderInspectorCollector::SetOverlayCoverage(float InCoverage)
{
	FScopeLock Lock(&SelectedTextureMutex);
	OverlayCoverage = FMath::Clamp(InCoverage, 0.0f, 1.0f);
}

void FFrameRenderInspectorCollector::SetRangeLocked(bool bLocked)
{
	FScopeLock Lock(&SelectedTextureMutex);
	bRangeLocked = bLocked;
}

void FFrameRenderInspectorCollector::SetManualRange(float InMin, float InMax)
{
	SetAutoRange(InMin, InMax);
}

void FFrameRenderInspectorCollector::SetAutoRange(float InMin, float InMax)
{
	FScopeLock Lock(&SelectedTextureMutex);
	const float ClampedMax = FMath::Max(InMin + UE_SMALL_NUMBER, InMax);
	AutoRangeMin = InMin;
	AutoRangeMax = ClampedMax;
	bHasAutoRange = true;
	UE_LOG(LogTemp, Log, TEXT("FrameRenderInspector Visible Range [%s]: Min=%g Max=%g"), *SelectedTextureName, AutoRangeMin, AutoRangeMax);
}

void FFrameRenderInspectorCollector::RequestVisibleRangeUpdate()
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!bRangeLocked)
	{
		bPendingVisibleRangeUpdate = true;
	}
}

void FFrameRenderInspectorCollector::RequestBufferCapture()
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!SelectedBufferName.IsEmpty())
	{
		bPendingBufferCapture = true;
	}
}

bool FFrameRenderInspectorCollector::ConsumeVisibleRangeUpdateRequest()
{
	FScopeLock Lock(&SelectedTextureMutex);
	const bool bWasPending = bPendingVisibleRangeUpdate;
	bPendingVisibleRangeUpdate = false;
	return bWasPending;
}

bool FFrameRenderInspectorCollector::ConsumeBufferCaptureRequest(FString& OutBufferName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!bPendingBufferCapture || SelectedBufferName.IsEmpty())
	{
		return false;
	}

	bPendingBufferCapture = false;
	OutBufferName = SelectedBufferName;
	return true;
}

void FFrameRenderInspectorCollector::PollAutoRangeReadback()
{
	if (!AutoRangeReadback.IsValid())
	{
		return;
	}

	bool bIsReady = false;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bIsReady = bAutoRangeReadbackPending && AutoRangeReadback->IsReady();
	}

	if (!bIsReady)
	{
		return;
	}

	const uint32* RangeData = static_cast<const uint32*>(AutoRangeReadback->Lock(sizeof(uint32) * 2));
	if (RangeData)
	{
		const float MinValue = DecodeOrderedFloat(RangeData[0]);
		const float MaxValue = DecodeOrderedFloat(RangeData[1]);
		if (FMath::IsFinite(MinValue) && FMath::IsFinite(MaxValue) && MinValue <= MaxValue)
		{
			SetAutoRange(MinValue, MaxValue);
		}
	}
	AutoRangeReadback->Unlock();

	FScopeLock Lock(&SelectedTextureMutex);
	bAutoRangeReadbackPending = false;
}

void FFrameRenderInspectorCollector::PollBufferReadback()
{
	if (!BufferReadback.IsValid())
	{
		return;
	}

	bool bIsReady = false;
	FString CompletedBufferName;
	uint32 CompletedStride = 0;
	uint32 CompletedCount = 0;
	uint32 CompletedNumBytes = 0;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bIsReady = bBufferReadbackPending && BufferReadback->IsReady();
		CompletedBufferName = BufferReadbackName;
		CompletedStride = BufferReadbackStride;
		CompletedCount = BufferReadbackCount;
		CompletedNumBytes = BufferReadbackNumBytes;
	}

	if (!bIsReady)
	{
		return;
	}

	FBufferReadbackResult Result;
	Result.Name = CompletedBufferName;
	Result.Stride = CompletedStride;
	Result.Count = CompletedCount;

	const void* LockedData = BufferReadback->Lock(CompletedNumBytes);
	if (LockedData && CompletedNumBytes > 0)
	{
		Result.Data.SetNumUninitialized(CompletedNumBytes);
		FMemory::Memcpy(Result.Data.GetData(), LockedData, CompletedNumBytes);
		Result.bSucceeded = true;
		Result.StatusMessage = FString::Printf(TEXT("Read back %u bytes from %s."), CompletedNumBytes, *CompletedBufferName);
	}
	else
	{
		Result.StatusMessage = FString::Printf(TEXT("Readback failed for %s."), *CompletedBufferName);
	}

	BufferReadback->Unlock();

	{
		FScopeLock Lock(&SelectedTextureMutex);
		bBufferReadbackPending = false;
		BufferReadbackName.Reset();
		BufferReadbackStride = 0;
		BufferReadbackCount = 0;
		BufferReadbackNumBytes = 0;
	}

	AsyncTask(ENamedThreads::GameThread, [Result = MoveTemp(Result)]() mutable
	{
		FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
		Module.UpdateBufferReadback(Result);
	});
}

FString FFrameRenderInspectorCollector::GetSelectedTextureName() const
{
	FScopeLock Lock(&SelectedTextureMutex);
	return SelectedTextureName;
}

FString FFrameRenderInspectorCollector::GetSelectedBufferName() const
{
	FScopeLock Lock(&SelectedTextureMutex);
	return SelectedBufferName;
}

void FFrameRenderInspectorCollector::GetRangeState(float& OutMin, float& OutMax, bool& bOutHasRange, bool& bOutRangeLocked) const
{
	FScopeLock Lock(&SelectedTextureMutex);
	OutMin = AutoRangeMin;
	OutMax = AutoRangeMax;
	bOutHasRange = bHasAutoRange;
	bOutRangeLocked = bRangeLocked;
}

void FFrameRenderInspectorCollector::GetOverlaySettings(float& OutOpacity, float& OutCoverage, float& OutAutoRangeMin, float& OutAutoRangeMax) const
{
	FScopeLock Lock(&SelectedTextureMutex);
	OutOpacity = OverlayOpacity;
	OutCoverage = OverlayCoverage;
	OutAutoRangeMin = AutoRangeMin;
	OutAutoRangeMax = AutoRangeMax;
}

static bool IsSingleChannelDebugFormat(EPixelFormat Format)
{
	switch (Format)
	{
	case PF_G8:
	case PF_G16:
	case PF_DepthStencil:
	case PF_ShadowDepth:
	case PF_D24:
	case PF_R32_FLOAT:
	case PF_R16F:
	case PF_R16F_FILTER:
	case PF_R16_UINT:
	case PF_R16_SINT:
	case PF_R32_UINT:
	case PF_R32_SINT:
	case PF_R8:
	case PF_R8_UINT:
	case PF_R8_SINT:
	case PF_L8:
		return true;
	default:
		return false;
	}
}

void FFrameRenderInspectorCollector::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	FAfterPassCallbackDelegateArray& InOutCallbacks,
	bool bIsPassEnabled)
{
	if (Pass != EPostProcessingPass::Tonemap)
	{
		return;
	}

	InOutCallbacks.Add(
		FAfterPassCallbackDelegate::CreateLambda(
			[this](FRDGBuilder& GraphBuilder,
				   const FSceneView& View,
				   const FPostProcessMaterialInputs& Inputs)
			{
				FScreenPassTexture SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
				const FString CurrentSelectedTexture = GetSelectedTextureName();

				if (!bIsCaptureEnabled.Load())
				{
					return SceneColor;
				}

				PollBufferReadback();

				TMap<FString, FIntPoint> CollectedInfo;
				TArray<FBufferDebuggerItem> BufferItems;
				FRDGTextureRef SelectedTexture = nullptr;
				FRDGBufferRef SelectedBuffer = nullptr;
				FString PendingBufferCaptureName;
				const bool bShouldCaptureBuffer = ConsumeBufferCaptureRequest(PendingBufferCaptureName);

				// Helper to add texture info
				auto AddTextureIfValid = [&](const FString& Name, FRDGTextureRef Tex)
				{
					if (Tex)
					{
						CollectedInfo.Add(Name, Tex->Desc.Extent);

						if (!CurrentSelectedTexture.IsEmpty() && Name == CurrentSelectedTexture)
						{
							SelectedTexture = Tex;
						}
					}
				};

				// --- 2. Collect from Render Target Pool (The "RHI" approach) ---
				// Use the new GetPooledTextureOwnershipMap()
				for (const auto& Pair : GraphBuilder.GetPooledTextureOwnershipMap())
				{
					FRDGPooledTexture* PooledTex = Pair.Key;
					FRDGTexture* RDGTex = Pair.Value;
					
					if (RDGTex && PooledTex)
					{
						FString Name = RDGTex->Name;
						if (Name.IsEmpty()) Name = TEXT("Unnamed_Pooled");
						if (!CollectedInfo.Contains(Name))
						{
							AddTextureIfValid(Name, RDGTex);
						}
					}
				}
				
				// Use the new GetExternalTextures()
				for (const auto& Pair : GraphBuilder.GetExternalTextures())
				{
					FRDGTexture* RDGTex = Pair.Value;
					if (RDGTex)
					{
						FString Name = RDGTex->Name;
						if (Name.IsEmpty()) Name = TEXT("External");
						if (!CollectedInfo.Contains(Name))
						{
							AddTextureIfValid(Name, RDGTex);
						}
					}
				}

				TSet<FString> SeenBufferNames;
				auto AddBufferIfValid = [&](FRDGBufferRef Buffer)
				{
					if (!Buffer)
					{
						return;
					}

					FString BufferName = Buffer->Name;
					if (BufferName.IsEmpty())
					{
						BufferName = TEXT("UnnamedBuffer");
					}

					if (SeenBufferNames.Contains(BufferName))
					{
						return;
					}

					SeenBufferNames.Add(BufferName);

					FBufferDebuggerItem Item;
					Item.Name = BufferName;
					Item.Stride = Buffer->Desc.BytesPerElement;
					Item.Count = Buffer->Desc.NumElements;
					Item.NumBytes = Buffer->Desc.GetSize();
					BufferItems.Add(Item);

					if (!PendingBufferCaptureName.IsEmpty() && BufferName == PendingBufferCaptureName)
					{
						SelectedBuffer = Buffer;
					}
				};

				EnumerateRDGBuffers(GraphBuilder, [&AddBufferIfValid](FRDGBuffer* Buffer)
				{
					AddBufferIfValid(Buffer);
				});

				if (SelectedTexture && SelectedTexture != SceneColor.Texture)
				{
					const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
					DrawSelectedTexturePreview(GraphBuilder, ViewInfo, SelectedTexture, SceneColor);
				}

				if (bShouldCaptureBuffer && SelectedBuffer && BufferReadback.IsValid())
				{
					const uint32 NumBytes = SelectedBuffer->Desc.GetSize();
					AddReadbackBufferPass(GraphBuilder, RDG_EVENT_NAME("FrameRenderInspector.BufferReadback"), SelectedBuffer,
						[this, SelectedBuffer, NumBytes](FRHICommandList& RHICmdList)
						{
							BufferReadback->EnqueueCopy(RHICmdList, SelectedBuffer->GetRHI(), NumBytes);
							FScopeLock Lock(&SelectedTextureMutex);
							bBufferReadbackPending = true;
							BufferReadbackName = SelectedBuffer->Name;
							BufferReadbackStride = SelectedBuffer->Desc.BytesPerElement;
							BufferReadbackCount = SelectedBuffer->Desc.NumElements;
							BufferReadbackNumBytes = NumBytes;
						});
				}

				if (CollectedInfo.Num() > 0 || BufferItems.Num() > 0)
				{
					AsyncTask(ENamedThreads::GameThread, [this, CollectedInfo, BufferItems]()
					{
						ProcessCollectedResources(CollectedInfo, BufferItems);
					});
				}

				return SceneColor;
			}));
}

void FFrameRenderInspectorCollector::ProcessCollectedResources(const TMap<FString, FIntPoint>& TextureInfo, const TArray<FBufferDebuggerItem>& BufferItems)
{
	TArray<FString> CurrentTextureNames;
	CurrentTextureNames.Reserve(TextureInfo.Num());
	for (const auto& Pair : TextureInfo)
	{
		CurrentTextureNames.Add(Pair.Key);
	}
	CurrentTextureNames.Sort();

	{
		FScopeLock Lock(&SelectedTextureMutex);
		if (CurrentTextureNames == LastCollectedTextureNames && BufferItems == LastCollectedBuffers)
		{
			return;
		}

		LastCollectedTextureNames = CurrentTextureNames;
		LastCollectedBuffers = BufferItems;
	}

	TArray<FTextureDebuggerItem> Items;
	for (const auto& Pair : TextureInfo)
	{
		FTextureDebuggerItem Item;
		Item.Name = Pair.Key;
		Item.Size = Pair.Value;
		Item.Format = TEXT("Unknown");
		Items.Add(Item);
	}

	FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
	Module.UpdateUI(Items, BufferItems);
}

void FFrameRenderInspectorCollector::DrawSelectedTexturePreview(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTexture* SelectedTexture,
	const FScreenPassTexture& SceneColor)
{
	if (!SelectedTexture || !SceneColor.Texture)
	{
		return;
	}

	PollAutoRangeReadback();

	const FIntPoint SourceExtent = SelectedTexture->Desc.Extent;
	const FIntRect SceneViewRect = SceneColor.ViewRect;
	float CurrentOverlayOpacity = 1.0f;
	float CurrentOverlayCoverage = 0.5f;
	float CurrentAutoRangeMin = 0.0f;
	float CurrentAutoRangeMax = 1.0f;
	GetOverlaySettings(CurrentOverlayOpacity, CurrentOverlayCoverage, CurrentAutoRangeMin, CurrentAutoRangeMax);
	const bool bVisualizeAsSingleChannel = IsSingleChannelDebugFormat(SelectedTexture->Desc.Format);
	bool bApplyRangeNormalization = false;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bApplyRangeNormalization = bHasAutoRange;
	}

	if (SourceExtent.X <= 0 || SourceExtent.Y <= 0 || SceneViewRect.Width() <= 0 || SceneViewRect.Height() <= 0 || CurrentOverlayOpacity <= 0.0f || CurrentOverlayCoverage <= 0.0f)
	{
		return;
	}

	const FIntPoint MaxPreviewSize(
		FMath::Clamp(FMath::FloorToInt(SceneViewRect.Width() * CurrentOverlayCoverage), 1, SceneViewRect.Width()),
		FMath::Clamp(FMath::FloorToInt(SceneViewRect.Height() * CurrentOverlayCoverage), 1, SceneViewRect.Height()));

	// Keep the debugger view pixel-aligned with the source view: same screen-space region,
	// copied 1:1 into the lower-left half of the viewport, with any overflow clipped away.
	FIntRect InputRect(
		SceneViewRect.Min.X,
		SceneViewRect.Max.Y - MaxPreviewSize.Y,
		SceneViewRect.Min.X + MaxPreviewSize.X,
		SceneViewRect.Max.Y);

	InputRect.Clip(FIntRect(FIntPoint::ZeroValue, SourceExtent));

	if (InputRect.Width() <= 0 || InputRect.Height() <= 0)
	{
		return;
	}

	const FIntRect OutputRect(
		SceneViewRect.Min.X,
		SceneViewRect.Max.Y - InputRect.Height(),
		SceneViewRect.Min.X + InputRect.Width(),
		SceneViewRect.Max.Y);

	if (ConsumeVisibleRangeUpdateRequest())
	{
		TStaticArray<uint32, 2> InitialRangeData;
		InitialRangeData[0] = EncodeOrderedFloat(FLT_MAX);
		InitialRangeData[1] = EncodeOrderedFloat(-FLT_MAX);

		FRDGBufferRef VisibleRangeBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("TextureFrame.VisibleRange"),
			sizeof(uint32),
			InitialRangeData.Num(),
			InitialRangeData.GetData(),
			sizeof(uint32) * InitialRangeData.Num());
		AddComputeVisibleRangePass(GraphBuilder, View, SelectedTexture, InputRect, bVisualizeAsSingleChannel, VisibleRangeBuffer);

		if (AutoRangeReadback.IsValid())
		{
			AddReadbackBufferPass(GraphBuilder, RDG_EVENT_NAME("TextureFrameVisibleRangeReadback"), VisibleRangeBuffer,
				[this, VisibleRangeBuffer](FRHICommandList& RHICmdList)
				{
					AutoRangeReadback->EnqueueCopy(RHICmdList, VisibleRangeBuffer->GetRHI(), 0u);
					FScopeLock Lock(&SelectedTextureMutex);
					bAutoRangeReadbackPending = true;
					bPendingVisibleRangeUpdate = false;
				});
		}
		else
		{
			FScopeLock Lock(&SelectedTextureMutex);
			bPendingVisibleRangeUpdate = false;
		}
	}

	AddFrameRenderInspectorOverlayPass(
		GraphBuilder,
		View,
		SelectedTexture,
		InputRect,
		SceneColor.Texture,
		OutputRect,
		CurrentOverlayOpacity,
		bApplyRangeNormalization,
		bVisualizeAsSingleChannel,
		CurrentAutoRangeMin,
		CurrentAutoRangeMax);
}
