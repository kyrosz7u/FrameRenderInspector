#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "FrameRenderInspectorTypes.h"
#include "FrameRenderInspectorCollector.h"

class FFrameRenderInspectorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Opens the debugger window
	void OpenDebuggerWindow();

	// Interface for other systems to update the UI
	// This should be called on the Game Thread
	void UpdateUI(const TArray<FTextureDebuggerItem>& TextureItems, const TArray<FBufferDebuggerItem>& BufferItems);
	void UpdateBufferReadback(const FBufferReadbackResult& ReadbackResult);
	void RefreshRenderOptions();

	// Called when a texture is selected in the UI
	void OnTextureSelected(const FString& TextureName);
	void OnBufferSelected(const FString& BufferName);
	void OnRefreshBufferRequested();
	void OnRenderOptionBoolChanged(const FString& OptionName, bool bValue);
	void OnRenderOptionValueCommitted(const FString& OptionName, const FString& ValueText);
	void OnOverlayOpacityChanged(float NewOpacity);
	void OnOverlayCoverageChanged(float NewCoverage);
	void OnComputeVisibleRangeRequested();
	void OnRangeLockChanged(bool bLocked);
	void OnRangeEdited(float NewMin, float NewMax);

	// Accessor for the collector
	TSharedPtr<class FFrameRenderInspectorCollector> GetCollector() { return Collector; }

private:
	void RegisterMenus();
	void UnregisterMenus();

	TSharedPtr<class SFrameRenderInspectorUI> DebuggerUI;
	TSharedPtr<class SDockTab> DebuggerTab;
	TSharedPtr<class FFrameRenderInspectorCollector> Collector;
	TArray<FString> CachedTextureNames;
	TArray<FBufferDebuggerItem> CachedBufferItems;
	TArray<FRenderOptionItem> CachedRenderOptions;
	FString SelectedTextureName;
	FString SelectedBufferName;
	FBufferReadbackResult LatestBufferReadback;
	bool bHasBufferReadback = false;
	float OverlayOpacity = 1.0f;
	float OverlayCoverage = 0.5f;
	float CurrentRangeMin = 0.0f;
	float CurrentRangeMax = 1.0f;
	bool bHasRange = false;
	bool bRangeLocked = false;
	bool bUIHasSyncedRangeState = false;
	
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	void OnDebuggerTabClosed(TSharedRef<class SDockTab> ClosedTab);
	void SetRDGImmediateModeEnabled(bool bEnabled) const;
	void EnsureCollectorInitialized();
};
