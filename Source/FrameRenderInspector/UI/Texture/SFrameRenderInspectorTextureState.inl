void SFrameRenderInspectorUI::UpdateTextureOptions(const TArray<FString>& TextureNames, const FString& SelectedTextureName)
{
	AllTextureOptions.Empty();
	for (const FString& TextureName : TextureNames)
	{
		AllTextureOptions.Add(MakeShared<FString>(TextureName));
	}

	SelectedTextureOption.Reset();
	for (const TSharedPtr<FString>& Option : AllTextureOptions)
	{
		if (Option.IsValid() && *Option == SelectedTextureName)
		{
			SelectedTextureOption = Option;
			break;
		}
	}

	RebuildFilteredOptions();
	ApplyFilteredTextureSelection();

	if (TextureComboBox.IsValid())
	{
		TextureComboBox->RefreshOptions();
		TextureComboBox->SetSelectedItem(SelectedTextureOption);
	}

	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetSelectedTextureName(SelectedTextureOption.IsValid() ? *SelectedTextureOption : FString());
	}
}

void SFrameRenderInspectorUI::SetTexturePreviewSize(const FIntPoint& InPreviewSize)
{
	TexturePreviewSize = InPreviewSize;
	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetPreviewSize(InPreviewSize);
	}
}

void SFrameRenderInspectorUI::SetTexturePixelSampleResult(const FTexturePixelSampleResult& InSampleResult)
{
	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetSampleResult(InSampleResult);
	}
}

void SFrameRenderInspectorUI::SetOnTextureSelected(FOnTextureSelected InOnTextureSelected)
{
	OnTextureSelectedDelegate = InOnTextureSelected;
}

void SFrameRenderInspectorUI::SetOnOverlayOpacityChanged(FOnOverlayOpacityChanged InOnOverlayOpacityChanged)
{
	OnOverlayOpacityChangedDelegate = InOnOverlayOpacityChanged;
}

void SFrameRenderInspectorUI::SetOnOverlayCoverageChanged(FOnOverlayCoverageChanged InOnOverlayCoverageChanged)
{
	OnOverlayCoverageChangedDelegate = InOnOverlayCoverageChanged;
}

void SFrameRenderInspectorUI::SetOnComputeVisibleRange(FOnComputeVisibleRange InOnComputeVisibleRange)
{
	OnComputeVisibleRangeDelegate = InOnComputeVisibleRange;
}

void SFrameRenderInspectorUI::SetOnRequestTexturePixelSample(FOnRequestTexturePixelSample InOnRequestTexturePixelSample)
{
	OnRequestTexturePixelSampleDelegate = InOnRequestTexturePixelSample;
	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetOnRequestPixelSample(
			SFrameRenderInspectorPixelPicker::FOnRequestPixelSample::CreateLambda([this](int32 PixelX, int32 PixelY)
			{
				if (OnRequestTexturePixelSampleDelegate.IsBound())
				{
					OnRequestTexturePixelSampleDelegate.Execute(PixelX, PixelY);
				}
			}));
	}
}

void SFrameRenderInspectorUI::SetOnBeginViewportTexturePick(FOnBeginViewportTexturePick InOnBeginViewportTexturePick)
{
	OnBeginViewportTexturePickDelegate = InOnBeginViewportTexturePick;
	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetOnBeginViewportPick(
			SFrameRenderInspectorPixelPicker::FOnBeginViewportPick::CreateLambda([this]()
			{
				if (OnBeginViewportTexturePickDelegate.IsBound())
				{
					OnBeginViewportTexturePickDelegate.Execute();
				}
			}));
	}
}

void SFrameRenderInspectorUI::SetViewportPickArmed(bool bInViewportPickArmed)
{
	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetViewportPickArmed(bInViewportPickArmed);
	}
}

void SFrameRenderInspectorUI::SetOverlaySettings(float InOpacity, float InCoverage)
{
	OverlayOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
	OverlayCoverage = FMath::Clamp(InCoverage, 0.0f, 1.0f);
}

void SFrameRenderInspectorUI::SetRangeState(float InMin, float InMax, bool bInHasRange, bool bInRangeLocked)
{
	RangeMin = InMin;
	RangeMax = InMax;
	bHasRange = bInHasRange;
	bRangeLocked = bInRangeLocked;
}

void SFrameRenderInspectorUI::RebuildFilteredOptions()
{
	FilteredTextureOptions.Empty();

	for (const TSharedPtr<FString>& Option : AllTextureOptions)
	{
		if (!Option.IsValid())
		{
			continue;
		}

		if (MatchesActiveMode(*Option) && (SearchText.IsEmpty() || Option->Contains(SearchText, ESearchCase::IgnoreCase)))
		{
			FilteredTextureOptions.Add(Option);
		}
	}

	if (SelectedTextureOption.IsValid() && !FilteredTextureOptions.Contains(SelectedTextureOption))
	{
		SelectedTextureOption.Reset();
	}
}

void SFrameRenderInspectorUI::ApplyFilteredTextureSelection()
{
	if (!SelectedTextureOption.IsValid() && FilteredTextureOptions.Num() > 0)
	{
		SelectedTextureOption = FilteredTextureOptions[0];
	}
}
