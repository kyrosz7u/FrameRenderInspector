#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "SceneViewExtension.h"

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
class FTextureFrameCollector : public FSceneViewExtensionBase
{
public:
	FTextureFrameCollector(const FAutoRegister& AutoRegister);
	virtual ~FTextureFrameCollector() {}

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
	void SetOverlayOpacity(float InOpacity);
	void SetOverlayCoverage(float InCoverage);
	void SetRangeLocked(bool bLocked);
	void SetManualRange(float InMin, float InMax);
	void RequestVisibleRangeUpdate();
	void GetRangeState(float& OutMin, float& OutMax, bool& bOutHasRange, bool& bOutRangeLocked) const;

private:
	TAtomic<bool> bIsCaptureEnabled;
	FString SelectedTextureName;
	float OverlayOpacity = 1.0f;
	float OverlayCoverage = 0.5f;
	float AutoRangeMin = 0.0f;
	float AutoRangeMax = 1.0f;
	bool bHasAutoRange = false;
	bool bRangeLocked = false;
	bool bPendingVisibleRangeUpdate = false;
	TUniquePtr<FRHIGPUBufferReadback> AutoRangeReadback;
	bool bAutoRangeReadbackPending = false;
	TArray<FString> LastCollectedTextureNames;
	mutable FCriticalSection SelectedTextureMutex;

	// Helper to process collected textures on the game thread
	void ProcessCollectedTextures(const TMap<FString, FIntPoint>& TextureInfo);
	FString GetSelectedTextureName() const;
	void GetOverlaySettings(float& OutOpacity, float& OutCoverage, float& OutAutoRangeMin, float& OutAutoRangeMax) const;
	void SetAutoRange(float InMin, float InMax);
	bool ConsumeVisibleRangeUpdateRequest();
	void PollAutoRangeReadback();
	void DrawSelectedTexturePreview(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTexture* SelectedTexture, const FScreenPassTexture& SceneColor);
};
