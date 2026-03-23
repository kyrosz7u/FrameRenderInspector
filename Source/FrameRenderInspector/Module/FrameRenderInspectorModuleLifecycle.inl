void FFrameRenderInspectorModule::StartupModule()
{
	LoadSettings();

	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FrameRenderInspector")))
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/FrameRenderInspector"), ShaderDirectory);
	}

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

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FrameRenderInspectorTabName);

	UnregisterMenus();
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
	DebuggerUI->SetOnRequestTexturePixelSample(SFrameRenderInspectorUI::FOnRequestTexturePixelSample::CreateRaw(this, &FFrameRenderInspectorModule::OnRequestTexturePixelSample));
	DebuggerUI->SetOnBeginViewportTexturePick(SFrameRenderInspectorUI::FOnBeginViewportTexturePick::CreateRaw(this, &FFrameRenderInspectorModule::OnBeginViewportTexturePick));
	DebuggerUI->SetOnRangeLockChanged(SFrameRenderInspectorUI::FOnRangeLockChanged::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeLockChanged));
	DebuggerUI->SetOnRangeEdited(SFrameRenderInspectorUI::FOnRangeEdited::CreateRaw(this, &FFrameRenderInspectorModule::OnRangeEdited));

	ApplySettingsToRuntime();
	DebuggerUI->SetTexturePreviewSize(CurrentTexturePreviewSize);

	if (bHasBufferReadback)
	{
		DebuggerUI->SetBufferReadbackResult(LatestBufferReadback);
	}
	if (bHasTexturePixelSample)
	{
		DebuggerUI->SetTexturePixelSampleResult(LatestTexturePixelSample);
	}

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
