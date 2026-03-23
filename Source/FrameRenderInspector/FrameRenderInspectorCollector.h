#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "SceneViewExtension.h"
#include "FrameRenderInspectorTypes.h"
#include "FrameRenderInspectorPixelPickerTypes.h"

// Forward declarations
class FRDGBuilder;
class FRDGTexture;
class FViewInfo;
class FRHIGPUBufferReadback;
struct FScreenPassTexture;
struct FPostProcessMaterialInputs;

/**
 * Collects Render Graph textures from the post-processing pipeline.
 */
class FFrameRenderInspectorCollector : public FSceneViewExtensionBase
{
public:
	FFrameRenderInspectorCollector(const FAutoRegister& AutoRegister);
	virtual ~FFrameRenderInspectorCollector() {}

	// ISceneViewExtension interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}

	virtual void SubscribeToPostProcessingPass(
		EPostProcessingPass Pass,
		FAfterPassCallbackDelegateArray& InOutCallbacks,
		bool bIsPassEnabled) override;

	// Enable or disable collection
	void SetCaptureEnabled(bool bEnabled);
	bool IsCaptureEnabled() const { return bIsCaptureEnabled.Load(); }

	// Set the texture to visualize
	void SetSelectedTexture(const FString& TextureName);
	void SetSelectedBuffer(const FString& BufferName);
	void SetVisualizeInViewport(bool bEnabled);
	void SetOverlayOpacity(float InOpacity);
	void SetOverlayCoverage(float InCoverage);
	void SetRangeLocked(bool bLocked);
	void SetManualRange(float InMin, float InMax);
	void RequestVisibleRangeUpdate();
	void RequestTexturePixelSample(int32 PixelX, int32 PixelY);
	void RequestBufferCapture();
	void GetRangeState(float& OutMin, float& OutMax, bool& bOutHasRange, bool& bOutRangeLocked) const;

private:
	TAtomic<bool> bIsCaptureEnabled;
	FString SelectedTextureName;
	FString SelectedBufferName;
	bool bVisualizeInViewport = true;
	float OverlayOpacity = 1.0f;
	float OverlayCoverage = 0.5f;
	float AutoRangeMin = 0.0f;
	float AutoRangeMax = 1.0f;
	bool bHasAutoRange = false;
	bool bRangeLocked = false;
	bool bPendingVisibleRangeUpdate = false;
	TUniquePtr<FRHIGPUBufferReadback> AutoRangeReadback;
	bool bAutoRangeReadbackPending = false;
	TUniquePtr<FRHIGPUBufferReadback> TexturePixelReadback;
	bool bTexturePixelReadbackPending = false;
	bool bPendingTexturePixelSample = false;
	FIntPoint PendingTexturePixel = FIntPoint::ZeroValue;
	FString TexturePixelReadbackName;
	FIntPoint TexturePixelReadbackPreviewSize = FIntPoint::ZeroValue;
	FIntPoint TexturePixelReadbackPixel = FIntPoint::ZeroValue;
	FIntPoint LastPreviewSize = FIntPoint::ZeroValue;
	TArray<FString> LastCollectedTextureNames;
	TArray<FBufferDebuggerItem> LastCollectedBuffers;
	TUniquePtr<FRHIGPUBufferReadback> BufferReadback;
	bool bBufferReadbackPending = false;
	bool bPendingBufferCapture = false;
	FString BufferReadbackName;
	uint32 BufferReadbackStride = 0;
	uint32 BufferReadbackCount = 0;
	uint32 BufferReadbackNumBytes = 0;
	mutable FCriticalSection SelectedTextureMutex;

	// Helper to process collected textures on the game thread
	void ProcessCollectedResources(const TMap<FString, FIntPoint>& TextureInfo, const TArray<FBufferDebuggerItem>& BufferItems);
	FString GetSelectedTextureName() const;
	FString GetSelectedBufferName() const;
	void GetOverlaySettings(bool& bOutVisualizeInViewport, float& OutOpacity, float& OutCoverage, float& OutAutoRangeMin, float& OutAutoRangeMax) const;
	void SetAutoRange(float InMin, float InMax);
	bool ConsumeVisibleRangeUpdateRequest();
	bool ConsumeTexturePixelSampleRequest(FIntPoint& OutPixelCoord);
	bool ConsumeBufferCaptureRequest(FString& OutBufferName);
	void PollAutoRangeReadback();
	void PollTexturePixelReadback();
	void PollBufferReadback();
	void DrawSelectedTexturePreview(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTexture* SelectedTexture, const FScreenPassTexture& SceneColor);
};
