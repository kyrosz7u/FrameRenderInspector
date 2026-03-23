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

void FFrameRenderInspectorModule::OnVisualizeInViewportChanged(bool bEnabled)
{
	bVisualizeInViewport = bEnabled;

	if (Collector.IsValid())
	{
		Collector->SetVisualizeInViewport(bVisualizeInViewport);
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
