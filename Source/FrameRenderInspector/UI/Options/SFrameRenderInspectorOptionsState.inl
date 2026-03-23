void SFrameRenderInspectorUI::UpdateRenderOptions(const TArray<FRenderOptionItem>& RenderOptions)
{
	AllRenderOptions.Empty();
	for (const FRenderOptionItem& RenderOption : RenderOptions)
	{
		AllRenderOptions.Add(MakeShared<FRenderOptionItem>(RenderOption));
	}

	RebuildFilteredRenderOptions();
}

void SFrameRenderInspectorUI::SetInspectorModeValue(int32 InModeValue)
{
	const int32 ClampedModeValue = FMath::Clamp(InModeValue, 0, static_cast<int32>(EInspectorMode::VirtualShadowMap));
	SetInspectorMode(static_cast<EInspectorMode>(ClampedModeValue));
}

void SFrameRenderInspectorUI::SetOnRenderOptionBoolChanged(FOnRenderOptionBoolChanged InOnRenderOptionBoolChanged)
{
	OnRenderOptionBoolChangedDelegate = InOnRenderOptionBoolChanged;
}

void SFrameRenderInspectorUI::SetOnRenderOptionValueCommitted(FOnRenderOptionValueCommitted InOnRenderOptionValueCommitted)
{
	OnRenderOptionValueCommittedDelegate = InOnRenderOptionValueCommitted;
}

int32 SFrameRenderInspectorUI::GetInspectorModeValue() const
{
	return static_cast<int32>(ActiveMode);
}

void SFrameRenderInspectorUI::RebuildFilteredRenderOptions()
{
	FilteredRenderOptions.Empty();

	for (const TSharedPtr<FRenderOptionItem>& Option : AllRenderOptions)
	{
		if (!Option.IsValid())
		{
			continue;
		}

		if (MatchesActiveMode(Option->Name) && (RenderOptionFilterText.IsEmpty() || Option->Name.Contains(RenderOptionFilterText, ESearchCase::IgnoreCase)))
		{
			FilteredRenderOptions.Add(Option);
		}
	}

	CurrentRenderOptionsPage = 0;
	RebuildVisibleRenderOptions();
}

void SFrameRenderInspectorUI::RebuildAllFilters()
{
	RebuildFilteredOptions();
	RebuildFilteredBufferOptions();
	RebuildFilteredRenderOptions();
}

void SFrameRenderInspectorUI::SetInspectorMode(EInspectorMode NewMode)
{
	if (ActiveMode == NewMode)
	{
		return;
	}

	ActiveMode = NewMode;
	RebuildAllFilters();
	ApplyFilteredTextureSelection();
	ApplyFilteredBufferSelection();

	if (TextureComboBox.IsValid())
	{
		TextureComboBox->RefreshOptions();
		TextureComboBox->SetSelectedItem(SelectedTextureOption);
	}

	if (BufferComboBox.IsValid())
	{
		BufferComboBox->RefreshOptions();
		bIsSyncingBufferSelection = true;
		BufferComboBox->SetSelectedItem(SelectedBufferOption);
		bIsSyncingBufferSelection = false;
	}
}

bool SFrameRenderInspectorUI::MatchesActiveMode(const FString& ResourceName) const
{
	const bool bIsLumenRelated = ContainsKeyword(ResourceName, LumenKeywords, UE_ARRAY_COUNT(LumenKeywords));
	const bool bIsNaniteRelated = ContainsKeyword(ResourceName, NaniteKeywords, UE_ARRAY_COUNT(NaniteKeywords));
	const bool bIsPostProcessRelated = ContainsKeyword(ResourceName, PostProcessKeywords, UE_ARRAY_COUNT(PostProcessKeywords));
	const bool bIsShadowRelated = ContainsKeyword(ResourceName, ShadowKeywords, UE_ARRAY_COUNT(ShadowKeywords));
	const bool bIsVirtualShadowMapRelated = ContainsKeyword(ResourceName, VirtualShadowMapKeywords, UE_ARRAY_COUNT(VirtualShadowMapKeywords));

	switch (ActiveMode)
	{
	case EInspectorMode::All:
		return true;
	case EInspectorMode::General:
		return !bIsLumenRelated && !bIsNaniteRelated && !bIsPostProcessRelated && !bIsShadowRelated && !bIsVirtualShadowMapRelated;
	case EInspectorMode::Lumen:
		return bIsLumenRelated;
	case EInspectorMode::Nanite:
		return bIsNaniteRelated;
	case EInspectorMode::PostProcess:
		return bIsPostProcessRelated;
	case EInspectorMode::Shadow:
		return bIsShadowRelated;
	case EInspectorMode::VirtualShadowMap:
		return bIsVirtualShadowMapRelated;
	default:
		return true;
	}
}

void SFrameRenderInspectorUI::RebuildVisibleRenderOptions()
{
	VisibleRenderOptions.Empty();
	ClampRenderOptionsPage();

	const int32 StartIndex = CurrentRenderOptionsPage * RenderOptionsPerPage;
	const int32 EndIndex = FMath::Min(StartIndex + RenderOptionsPerPage, FilteredRenderOptions.Num());
	for (int32 Index = StartIndex; Index < EndIndex; ++Index)
	{
		VisibleRenderOptions.Add(FilteredRenderOptions[Index]);
	}

	if (RenderOptionsView.IsValid())
	{
		RenderOptionsView->RequestListRefresh();
	}
}
