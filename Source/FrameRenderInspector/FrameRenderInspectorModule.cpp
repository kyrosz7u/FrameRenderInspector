#include "FrameRenderInspectorModule.h"
#include "SFrameRenderInspectorUI.h"
#include "FrameRenderInspectorCollector.h"
#include "FrameRenderInspectorPixelPickerModule.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

static const FName FrameRenderInspectorTabName("FrameRenderInspector");
static const TCHAR* FrameRenderInspectorSettingsSection = TEXT("/Script/FrameRenderInspector.FrameRenderInspectorSettings");

#define LOCTEXT_NAMESPACE "FFrameRenderInspectorModule"

void FFrameRenderInspectorModule::StartupModule()
{
	LoadSettings();

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
	SaveSettings();

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

void FFrameRenderInspectorModule::LoadSettings()
{
	LoadSettingsFromFile(GEditorPerProjectIni);
}

bool FFrameRenderInspectorModule::LoadSettingsFromFile(const FString& ConfigPath)
{
	if (!GConfig)
	{
		return false;
	}

	FConfigFile ConfigFile;
	ConfigFile.Read(ConfigPath);

	const FConfigSection* SettingsSection = ConfigFile.Find(FrameRenderInspectorSettingsSection);
	if (!SettingsSection)
	{
		return false;
	}

	auto GetConfigString = [SettingsSection](const TCHAR* Key, FString& OutValue)
	{
		if (const FConfigValue* ConfigValue = SettingsSection->Find(Key))
		{
			OutValue = ConfigValue->GetValue();
		}
	};

	auto GetConfigFloat = [SettingsSection](const TCHAR* Key, float& OutValue)
	{
		if (const FConfigValue* ConfigValue = SettingsSection->Find(Key))
		{
			LexTryParseString(OutValue, *ConfigValue->GetValue());
		}
	};

	auto GetConfigInt = [SettingsSection](const TCHAR* Key, int32& OutValue)
	{
		if (const FConfigValue* ConfigValue = SettingsSection->Find(Key))
		{
			LexTryParseString(OutValue, *ConfigValue->GetValue());
		}
	};

	auto GetConfigBool = [SettingsSection](const TCHAR* Key, bool& OutValue)
	{
		if (const FConfigValue* ConfigValue = SettingsSection->Find(Key))
		{
			OutValue = ConfigValue->GetValue().ToBool();
		}
	};

	GetConfigString(TEXT("SelectedTexture"), SelectedTextureName);
	GetConfigString(TEXT("SelectedBuffer"), SelectedBufferName);
	GetConfigFloat(TEXT("OverlayOpacity"), OverlayOpacity);
	GetConfigFloat(TEXT("OverlayCoverage"), OverlayCoverage);
	GetConfigFloat(TEXT("RangeMin"), CurrentRangeMin);
	GetConfigFloat(TEXT("RangeMax"), CurrentRangeMax);
	GetConfigBool(TEXT("HasRange"), bHasRange);
	GetConfigBool(TEXT("RangeLocked"), bRangeLocked);
	GetConfigInt(TEXT("InspectorMode"), InspectorModeValue);
	GetConfigInt(TEXT("BufferRows"), BufferRowsSetting);
	GetConfigInt(TEXT("BufferColumns"), BufferColumnsSetting);
	GetConfigString(TEXT("BufferFormat"), BufferFormatName);

	OverlayOpacity = FMath::Clamp(OverlayOpacity, 0.0f, 1.0f);
	OverlayCoverage = FMath::Clamp(OverlayCoverage, 0.0f, 1.0f);
	BufferRowsSetting = FMath::Clamp(BufferRowsSetting, 1, 128);
	BufferColumnsSetting = FMath::Clamp(BufferColumnsSetting, 1, 64);
	if (BufferFormatName.IsEmpty())
	{
		BufferFormatName = TEXT("UInt");
	}
	return true;
}

void FFrameRenderInspectorModule::SaveSettings()
{
	SaveSettingsToFile(GEditorPerProjectIni);
}

bool FFrameRenderInspectorModule::SaveSettingsToFile(const FString& ConfigPath)
{
	if (!GConfig)
	{
		return false;
	}

	if (DebuggerUI.IsValid())
	{
		InspectorModeValue = DebuggerUI->GetInspectorModeValue();
		BufferRowsSetting = DebuggerUI->GetBufferRowsSetting();
		BufferColumnsSetting = DebuggerUI->GetBufferColumnsSetting();
		BufferFormatName = DebuggerUI->GetBufferFormatName();
	}

	FConfigFile ConfigFile;
	FConfigSection& SettingsSection = ConfigFile.FindOrAdd(FrameRenderInspectorSettingsSection);
	SettingsSection.Remove(TEXT("SelectedTexture"));
	SettingsSection.Remove(TEXT("SelectedBuffer"));
	SettingsSection.Remove(TEXT("OverlayOpacity"));
	SettingsSection.Remove(TEXT("OverlayCoverage"));
	SettingsSection.Remove(TEXT("RangeMin"));
	SettingsSection.Remove(TEXT("RangeMax"));
	SettingsSection.Remove(TEXT("HasRange"));
	SettingsSection.Remove(TEXT("RangeLocked"));
	SettingsSection.Remove(TEXT("InspectorMode"));
	SettingsSection.Remove(TEXT("BufferRows"));
	SettingsSection.Remove(TEXT("BufferColumns"));
	SettingsSection.Remove(TEXT("BufferFormat"));

	SettingsSection.Add(TEXT("SelectedTexture"), FConfigValue(SelectedTextureName));
	SettingsSection.Add(TEXT("SelectedBuffer"), FConfigValue(SelectedBufferName));
	SettingsSection.Add(TEXT("OverlayOpacity"), FConfigValue(FString::SanitizeFloat(OverlayOpacity)));
	SettingsSection.Add(TEXT("OverlayCoverage"), FConfigValue(FString::SanitizeFloat(OverlayCoverage)));
	SettingsSection.Add(TEXT("RangeMin"), FConfigValue(FString::SanitizeFloat(CurrentRangeMin)));
	SettingsSection.Add(TEXT("RangeMax"), FConfigValue(FString::SanitizeFloat(CurrentRangeMax)));
	SettingsSection.Add(TEXT("HasRange"), FConfigValue(bHasRange ? TEXT("True") : TEXT("False")));
	SettingsSection.Add(TEXT("RangeLocked"), FConfigValue(bRangeLocked ? TEXT("True") : TEXT("False")));
	SettingsSection.Add(TEXT("InspectorMode"), FConfigValue(LexToString(InspectorModeValue)));
	SettingsSection.Add(TEXT("BufferRows"), FConfigValue(LexToString(BufferRowsSetting)));
	SettingsSection.Add(TEXT("BufferColumns"), FConfigValue(LexToString(BufferColumnsSetting)));
	SettingsSection.Add(TEXT("BufferFormat"), FConfigValue(BufferFormatName));

	const bool bSaved = ConfigFile.Write(ConfigPath);

	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("FrameRenderInspector config saved to: %s"), *ConfigPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FrameRenderInspector config save failed: %s"), *ConfigPath);
	}

	return bSaved;
}

void FFrameRenderInspectorModule::ApplySettingsToRuntime()
{
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

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
		DebuggerUI->UpdateBufferOptions(CachedBufferItems, SelectedBufferName);
		DebuggerUI->UpdateRenderOptions(CachedRenderOptions);
		DebuggerUI->SetInspectorModeValue(InspectorModeValue);
		DebuggerUI->SetBufferViewSettings(BufferRowsSetting, BufferColumnsSetting, BufferFormatName);
		DebuggerUI->SetOverlaySettings(OverlayOpacity, OverlayCoverage);
		DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
	}
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
	DebuggerUI->SetOnRequestTexturePixelSample(SFrameRenderInspectorUI::FOnRequestTexturePixelSample::CreateRaw(this, &FFrameRenderInspectorModule::OnRequestTexturePixelSample));
	DebuggerUI->SetOnBeginViewportTexturePick(SFrameRenderInspectorUI::FOnBeginViewportTexturePick::CreateRaw(this, &FFrameRenderInspectorModule::OnBeginViewportTexturePick));
	DebuggerUI->SetOnRangeLockChanged(SFrameRenderInspectorUI::FOnRangeLockChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeLockChanged));
	DebuggerUI->SetOnRangeEdited(SFrameRenderInspectorUI::FOnRangeEdited::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeEdited));
	DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
	DebuggerUI->UpdateBufferOptions(CachedBufferItems, SelectedBufferName);
	DebuggerUI->UpdateRenderOptions(CachedRenderOptions);
	DebuggerUI->SetInspectorModeValue(InspectorModeValue);
	DebuggerUI->SetBufferViewSettings(BufferRowsSetting, BufferColumnsSetting, BufferFormatName);
	DebuggerUI->SetTexturePreviewSize(CurrentTexturePreviewSize);
	DebuggerUI->SetOverlaySettings(OverlayOpacity, OverlayCoverage);
	DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
	if (bHasBufferReadback)
	{
		DebuggerUI->SetBufferReadbackResult(LatestBufferReadback);
	}
	if (bHasTexturePixelSample)
	{
		DebuggerUI->SetTexturePixelSampleResult(LatestTexturePixelSample);
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

void FFrameRenderInspectorModule::UpdateTexturePreviewSize(const FIntPoint& PreviewSize)
{
	CurrentTexturePreviewSize = PreviewSize;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetTexturePreviewSize(PreviewSize);
	}
}

void FFrameRenderInspectorModule::UpdateTexturePixelSample(const FTexturePixelSampleResult& SampleResult)
{
	LatestTexturePixelSample = SampleResult;
	bHasTexturePixelSample = true;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetTexturePixelSampleResult(SampleResult);
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

void FFrameRenderInspectorModule::OnRequestTexturePixelSample(int32 PixelX, int32 PixelY)
{
	if (Collector.IsValid())
	{
		Collector->RequestTexturePixelSample(PixelX, PixelY);
	}
}

void FFrameRenderInspectorModule::OnBeginViewportTexturePick()
{
	FFrameRenderInspectorPixelPickerModule& PixelPickerModule =
		FModuleManager::LoadModuleChecked<FFrameRenderInspectorPixelPickerModule>("FrameRenderInspectorPixelPicker");

	if (CurrentTexturePreviewSize.X <= 0 || CurrentTexturePreviewSize.Y <= 0)
	{
		if (DebuggerUI.IsValid())
		{
			DebuggerUI->SetViewportPickArmed(false);
		}
		return;
	}

	PixelPickerModule.ArmViewportPicker(
		CurrentTexturePreviewSize,
		FFrameRenderInspectorPixelPickerModule::FOnViewportPixelPicked::CreateRaw(this, &FFrameRenderInspectorModule::OnViewportTexturePickCompleted));

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetViewportPickArmed(PixelPickerModule.IsViewportPickerArmed());
	}
}

void FFrameRenderInspectorModule::OnViewportTexturePickCompleted(bool bSucceeded, int32 PixelX, int32 PixelY)
{
	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetViewportPickArmed(false);
	}

	if (bSucceeded && Collector.IsValid())
	{
		Collector->RequestTexturePixelSample(PixelX, PixelY);
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
