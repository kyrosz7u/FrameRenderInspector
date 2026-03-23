#include "FrameRenderInspectorModule.h"
#include "SFrameRenderInspectorUI.h"
#include "FrameRenderInspectorCollector.h"
#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

static const FName FrameRenderInspectorTabName("FrameRenderInspector");

#define LOCTEXT_NAMESPACE "FFrameRenderInspectorModule"

void FFrameRenderInspectorModule::StartupModule()
{
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FrameRenderInspector")))
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/FrameRenderInspector"), ShaderDirectory);
	}

	// Register the tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FrameRenderInspectorTabName, FOnSpawnTab::CreateRaw(this, &FFrameRenderInspectorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FrameRenderInspectorTabTitle", "Frame Render Inspector"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FFrameRenderInspectorModule::RegisterMenus));
}

void FFrameRenderInspectorModule::ShutdownModule()
{
	if (DebuggerTab.IsValid())
	{
		DebuggerTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback());
		DebuggerTab.Reset();
	}

	DebuggerUI.Reset();
	SetRDGImmediateModeEnabled(false);

	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FrameRenderInspectorTabName);

	UnregisterMenus();
	
	// Cleanup collector
	Collector.Reset();
}

void FFrameRenderInspectorModule::RegisterMenus()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}

	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection("DebugTools");
	Section.AddMenuEntry(
		"OpenFrameRenderInspector",
		LOCTEXT("OpenFrameRenderInspectorLabel", "Frame Render Inspector"),
		LOCTEXT("OpenFrameRenderInspectorTooltip", "Open the Frame Render Inspector window."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FFrameRenderInspectorModule::OpenDebuggerWindow)));
}

void FFrameRenderInspectorModule::UnregisterMenus()
{
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		ToolMenus->RemoveEntry("LevelEditor.MainMenu.Tools", "DebugTools", "OpenFrameRenderInspector");
		UToolMenus::UnregisterOwner(this);
	}
}

TSharedRef<SDockTab> FFrameRenderInspectorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	EnsureCollectorInitialized();

	DebuggerUI = SNew(SFrameRenderInspectorUI);
	DebuggerUI->SetOnTextureSelected(SFrameRenderInspectorUI::FOnTextureSelected::CreateRaw(this, &FFrameRenderInspectorModule::OnTextureSelected));
	DebuggerUI->SetOnBufferSelected(SFrameRenderInspectorUI::FOnBufferSelected::CreateRaw(this, &FFrameRenderInspectorModule::OnBufferSelected));
	DebuggerUI->SetOnRefreshBuffer(SFrameRenderInspectorUI::FOnRefreshBuffer::CreateRaw(this, &FFrameRenderInspectorModule::OnRefreshBufferRequested));
	DebuggerUI->SetOnRenderOptionBoolChanged(SFrameRenderInspectorUI::FOnRenderOptionBoolChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnRenderOptionBoolChanged));
	DebuggerUI->SetOnRenderOptionValueCommitted(SFrameRenderInspectorUI::FOnRenderOptionValueCommitted::CreateRaw(this, &FFrameRenderInspectorModule::OnRenderOptionValueCommitted));
	DebuggerUI->SetOnOverlayOpacityChanged(SFrameRenderInspectorUI::FOnOverlayOpacityChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnOverlayOpacityChanged));
	DebuggerUI->SetOnOverlayCoverageChanged(SFrameRenderInspectorUI::FOnOverlayCoverageChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnOverlayCoverageChanged));
	DebuggerUI->SetOnComputeVisibleRange(SFrameRenderInspectorUI::FOnComputeVisibleRange::CreateRaw(this, &FFrameRenderInspectorModule::OnComputeVisibleRangeRequested));
	DebuggerUI->SetOnRangeLockChanged(SFrameRenderInspectorUI::FOnRangeLockChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeLockChanged));
	DebuggerUI->SetOnRangeEdited(SFrameRenderInspectorUI::FOnRangeEdited::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeEdited));
	DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
	DebuggerUI->UpdateBufferOptions(CachedBufferItems, SelectedBufferName);
	DebuggerUI->UpdateRenderOptions(CachedRenderOptions);
	DebuggerUI->SetOverlaySettings(OverlayOpacity, OverlayCoverage);
	DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
	if (bHasBufferReadback)
	{
		DebuggerUI->SetBufferReadbackResult(LatestBufferReadback);
	}

	// Enable capture when window is opened
	if (Collector.IsValid())
	{
		Collector->SetCaptureEnabled(true);
		Collector->SetOverlayOpacity(OverlayOpacity);
		Collector->SetOverlayCoverage(OverlayCoverage);
		Collector->SetSelectedBuffer(SelectedBufferName);
		Collector->SetRangeLocked(bRangeLocked);
	}

	RefreshRenderOptions();
	SetRDGImmediateModeEnabled(true);

	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FFrameRenderInspectorModule::OnDebuggerTabClosed))
		[
			DebuggerUI.ToSharedRef()
		];

	DebuggerTab = NewTab;
	return NewTab;
}

void FFrameRenderInspectorModule::OpenDebuggerWindow()
{
	EnsureCollectorInitialized();

	if (!FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FrameRenderInspectorTabName)).IsValid())
	{
		DebuggerTab.Reset();
	}

	DebuggerTab = FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FrameRenderInspectorTabName));
}

void FFrameRenderInspectorModule::UpdateUI(const TArray<FTextureDebuggerItem>& TextureItems, const TArray<FBufferDebuggerItem>& BufferItems)
{
	TArray<FString> TextureNames;
	TextureNames.Reserve(TextureItems.Num());

	for (const FTextureDebuggerItem& Item : TextureItems)
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

	CachedBufferItems = BufferItems;
	CachedBufferItems.Sort([](const FBufferDebuggerItem& A, const FBufferDebuggerItem& B)
	{
		return A.Name < B.Name;
	});

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->UpdateBufferOptions(CachedBufferItems, SelectedBufferName);
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

void FFrameRenderInspectorModule::UpdateBufferReadback(const FBufferReadbackResult& ReadbackResult)
{
	LatestBufferReadback = ReadbackResult;
	bHasBufferReadback = true;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetBufferReadbackResult(ReadbackResult);
	}
}

void FFrameRenderInspectorModule::RefreshRenderOptions()
{
	CachedRenderOptions.Empty();

	IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
		FConsoleObjectVisitor::CreateLambda([this](const TCHAR* Name, IConsoleObject* Obj)
		{
			if (!Name || !Obj)
			{
				return;
			}

			IConsoleVariable* CVar = Obj->AsVariable();
			if (!CVar)
			{
				return;
			}

			FRenderOptionItem Item;
			Item.Name = Name;

			if (Obj->IsVariableBool())
			{
				Item.ValueType = ERenderOptionValueType::Bool;
				Item.bBoolValue = CVar->GetBool();
				Item.ValueText = Item.bBoolValue ? TEXT("1") : TEXT("0");
			}
			else if (Obj->IsVariableInt())
			{
				Item.ValueType = ERenderOptionValueType::Int;
				Item.ValueText = FString::Printf(TEXT("%d"), CVar->GetInt());
			}
			else
			{
				Item.ValueType = ERenderOptionValueType::Float;
				Item.ValueText = FString::Printf(TEXT("%g"), CVar->GetFloat());
			}

			CachedRenderOptions.Add(MoveTemp(Item));
		}),
		TEXT("r."));

	CachedRenderOptions.Sort([](const FRenderOptionItem& A, const FRenderOptionItem& B)
	{
		return A.Name < B.Name;
	});

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->UpdateRenderOptions(CachedRenderOptions);
	}
}

void FFrameRenderInspectorModule::OnTextureSelected(const FString& TextureName)
{
	SelectedTextureName = TextureName;
	
	if (Collector.IsValid())
	{
		Collector->SetSelectedTexture(TextureName);
	}
}

void FFrameRenderInspectorModule::OnBufferSelected(const FString& BufferName)
{
	SelectedBufferName = BufferName;

	if (Collector.IsValid())
	{
		Collector->SetSelectedBuffer(BufferName);
	}
}

void FFrameRenderInspectorModule::OnRefreshBufferRequested()
{
	if (Collector.IsValid())
	{
		Collector->RequestBufferCapture();
	}
}

void FFrameRenderInspectorModule::OnRenderOptionBoolChanged(const FString& OptionName, bool bValue)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*OptionName))
	{
		CVar->Set(bValue ? 1 : 0, ECVF_SetByConsole);
	}

	RefreshRenderOptions();
}

void FFrameRenderInspectorModule::OnRenderOptionValueCommitted(const FString& OptionName, const FString& ValueText)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*OptionName))
	{
		CVar->Set(*ValueText, ECVF_SetByConsole);
	}

	RefreshRenderOptions();
}

void FFrameRenderInspectorModule::OnOverlayOpacityChanged(float NewOpacity)
{
	OverlayOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);

	if (Collector.IsValid())
	{
		Collector->SetOverlayOpacity(OverlayOpacity);
	}
}

void FFrameRenderInspectorModule::OnOverlayCoverageChanged(float NewCoverage)
{
	OverlayCoverage = FMath::Clamp(NewCoverage, 0.0f, 1.0f);

	if (Collector.IsValid())
	{
		Collector->SetOverlayCoverage(OverlayCoverage);
	}
}

void FFrameRenderInspectorModule::OnComputeVisibleRangeRequested()
{
	if (Collector.IsValid())
	{
		Collector->RequestVisibleRangeUpdate();
	}
}

void FFrameRenderInspectorModule::OnRangeLockChanged(bool bLocked)
{
	bRangeLocked = bLocked;

	if (Collector.IsValid())
	{
		Collector->SetRangeLocked(bRangeLocked);
	}
}

void FFrameRenderInspectorModule::OnRangeEdited(float NewMin, float NewMax)
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

void FFrameRenderInspectorModule::OnDebuggerTabClosed(TSharedRef<SDockTab> ClosedTab)
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

void FFrameRenderInspectorModule::SetRDGImmediateModeEnabled(bool bEnabled) const
{
	if (IConsoleVariable* CVarRDGImmediate = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RDG.ImmediateMode")))
	{
		CVarRDGImmediate->Set(bEnabled ? 1 : 0, ECVF_SetByCode);
	}
}

void FFrameRenderInspectorModule::EnsureCollectorInitialized()
{
	if (!Collector.IsValid() && GEngine)
	{
		Collector = FSceneViewExtensions::NewExtension<FFrameRenderInspectorCollector>();

		if (Collector.IsValid())
		{
			Collector->SetSelectedTexture(SelectedTextureName);
			Collector->SetSelectedBuffer(SelectedBufferName);
			Collector->SetOverlayOpacity(OverlayOpacity);
			Collector->SetOverlayCoverage(OverlayCoverage);
			Collector->SetRangeLocked(bRangeLocked);
			if (bHasRange)
			{
				Collector->SetManualRange(CurrentRangeMin, CurrentRangeMax);
			}
		}
	}

	RefreshRenderOptions();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFrameRenderInspectorModule, FrameRenderInspector)
