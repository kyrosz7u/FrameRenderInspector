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
