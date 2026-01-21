#include "TextureFrameDebuggerModule.h"
#include "STextureFrameDebuggerUI.h"
#include "TextureFrameCollector.h"
#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

static const FName TextureFrameDebuggerTabName("TextureFrameDebugger");

#define LOCTEXT_NAMESPACE "FTextureFrameDebuggerModule"

void FTextureFrameDebuggerModule::StartupModule()
{
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TextureFrameDebugger")))
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/TextureFrameDebugger"), ShaderDirectory);
	}

	// Register the tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TextureFrameDebuggerTabName, FOnSpawnTab::CreateRaw(this, &FTextureFrameDebuggerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("TextureFrameDebuggerTabTitle", "Texture Frame Debugger"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FTextureFrameDebuggerModule::ShutdownModule()
{
	if (DebuggerTab.IsValid())
	{
		DebuggerTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback());
		DebuggerTab.Reset();
	}

	DebuggerUI.Reset();
	SetRDGImmediateModeEnabled(false);

	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TextureFrameDebuggerTabName);
	
	// Cleanup collector
	Collector.Reset();
}

TSharedRef<SDockTab> FTextureFrameDebuggerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	EnsureCollectorInitialized();

	DebuggerUI = SNew(STextureFrameDebuggerUI);
	DebuggerUI->SetOnTextureSelected(STextureFrameDebuggerUI::FOnTextureSelected::CreateRaw(this, &FTextureFrameDebuggerModule::OnTextureSelected));
	DebuggerUI->SetOnOverlayOpacityChanged(STextureFrameDebuggerUI::FOnOverlayOpacityChanged::CreateRaw(this, &FTextureFrameDebuggerModule::OnOverlayOpacityChanged));
	DebuggerUI->SetOnOverlayCoverageChanged(STextureFrameDebuggerUI::FOnOverlayCoverageChanged::CreateRaw(this, &FTextureFrameDebuggerModule::OnOverlayCoverageChanged));
	DebuggerUI->SetOnComputeVisibleRange(STextureFrameDebuggerUI::FOnComputeVisibleRange::CreateRaw(this, &FTextureFrameDebuggerModule::OnComputeVisibleRangeRequested));
	DebuggerUI->SetOnRangeLockChanged(STextureFrameDebuggerUI::FOnRangeLockChanged::CreateRaw(this, &FTextureFrameDebuggerModule::OnRangeLockChanged));
	DebuggerUI->SetOnRangeEdited(STextureFrameDebuggerUI::FOnRangeEdited::CreateRaw(this, &FTextureFrameDebuggerModule::OnRangeEdited));
	DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
	DebuggerUI->SetOverlaySettings(OverlayOpacity, OverlayCoverage);
	DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);

	// Enable capture when window is opened
	if (Collector.IsValid())
	{
		Collector->SetCaptureEnabled(true);
		Collector->SetOverlayOpacity(OverlayOpacity);
		Collector->SetOverlayCoverage(OverlayCoverage);
		Collector->SetRangeLocked(bRangeLocked);
	}

	SetRDGImmediateModeEnabled(true);

	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FTextureFrameDebuggerModule::OnDebuggerTabClosed))
		[
			DebuggerUI.ToSharedRef()
		];

	DebuggerTab = NewTab;
	return NewTab;
}

void FTextureFrameDebuggerModule::OpenDebuggerWindow()
{
	EnsureCollectorInitialized();

	if (!FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(TextureFrameDebuggerTabName)).IsValid())
	{
		DebuggerTab.Reset();
	}

	DebuggerTab = FGlobalTabmanager::Get()->TryInvokeTab(FTabId(TextureFrameDebuggerTabName));
}

void FTextureFrameDebuggerModule::UpdateUI(const TArray<FTextureDebuggerItem>& Items)
{
	TArray<FString> TextureNames;
	TextureNames.Reserve(Items.Num());

	for (const FTextureDebuggerItem& Item : Items)
	{
		TextureNames.Add(Item.Name);
	}

	TextureNames.Sort();
	if (TextureNames != CachedTextureNames)
	{
		CachedTextureNames = MoveTemp(TextureNames);

		if (DebuggerUI.IsValid())
		{
			DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
		}
	}

	if (Collector.IsValid())
	{
		const float PreviousRangeMin = CurrentRangeMin;
		const float PreviousRangeMax = CurrentRangeMax;
		const bool bPreviousHasRange = bHasRange;
		const bool bPreviousRangeLocked = bRangeLocked;

		Collector->GetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
		const bool bRangeStateChanged =
			!bUIHasSyncedRangeState ||
			!FMath::IsNearlyEqual(CurrentRangeMin, PreviousRangeMin) ||
			!FMath::IsNearlyEqual(CurrentRangeMax, PreviousRangeMax) ||
			bHasRange != bPreviousHasRange ||
			bRangeLocked != bPreviousRangeLocked;

		if (DebuggerUI.IsValid() && bRangeStateChanged)
		{
			DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
			bUIHasSyncedRangeState = true;
		}
	}
}

void FTextureFrameDebuggerModule::OnTextureSelected(const FString& TextureName)
{
	SelectedTextureName = TextureName;
	
	if (Collector.IsValid())
	{
		Collector->SetSelectedTexture(TextureName);
	}
}

void FTextureFrameDebuggerModule::OnOverlayOpacityChanged(float NewOpacity)
{
	OverlayOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);

	if (Collector.IsValid())
	{
		Collector->SetOverlayOpacity(OverlayOpacity);
	}
}

void FTextureFrameDebuggerModule::OnOverlayCoverageChanged(float NewCoverage)
{
	OverlayCoverage = FMath::Clamp(NewCoverage, 0.0f, 1.0f);

	if (Collector.IsValid())
	{
		Collector->SetOverlayCoverage(OverlayCoverage);
	}
}

void FTextureFrameDebuggerModule::OnComputeVisibleRangeRequested()
{
	if (Collector.IsValid())
	{
		Collector->RequestVisibleRangeUpdate();
	}
}

void FTextureFrameDebuggerModule::OnRangeLockChanged(bool bLocked)
{
	bRangeLocked = bLocked;

	if (Collector.IsValid())
	{
		Collector->SetRangeLocked(bRangeLocked);
	}
}

void FTextureFrameDebuggerModule::OnRangeEdited(float NewMin, float NewMax)
{
	CurrentRangeMin = NewMin;
	CurrentRangeMax = NewMax;
	bHasRange = true;
	bUIHasSyncedRangeState = true;

	if (Collector.IsValid())
	{
		Collector->SetManualRange(NewMin, NewMax);
	}
}

void FTextureFrameDebuggerModule::OnDebuggerTabClosed(TSharedRef<SDockTab> ClosedTab)
{
	ClosedTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback());

	if (Collector.IsValid())
	{
		Collector->SetCaptureEnabled(false);
	}

	SetRDGImmediateModeEnabled(false);
	bUIHasSyncedRangeState = false;
	DebuggerUI.Reset();
	DebuggerTab.Reset();
}

void FTextureFrameDebuggerModule::SetRDGImmediateModeEnabled(bool bEnabled) const
{
	if (IConsoleVariable* CVarRDGImmediate = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RDG.ImmediateMode")))
	{
		CVarRDGImmediate->Set(bEnabled ? 1 : 0, ECVF_SetByCode);
	}
}

void FTextureFrameDebuggerModule::EnsureCollectorInitialized()
{
	if (!Collector.IsValid() && GEngine)
	{
		Collector = FSceneViewExtensions::NewExtension<FTextureFrameCollector>();

		if (Collector.IsValid())
		{
			Collector->SetSelectedTexture(SelectedTextureName);
			Collector->SetOverlayOpacity(OverlayOpacity);
			Collector->SetOverlayCoverage(OverlayCoverage);
			Collector->SetRangeLocked(bRangeLocked);
			if (bHasRange)
			{
				Collector->SetManualRange(CurrentRangeMin, CurrentRangeMax);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTextureFrameDebuggerModule, TextureFrameDebugger)
