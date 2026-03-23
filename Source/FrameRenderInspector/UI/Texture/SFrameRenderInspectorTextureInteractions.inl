void SFrameRenderInspectorUI::OnTextureSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	SelectedTextureOption = Item;

	if (TexturePixelPickerWidget.IsValid())
	{
		TexturePixelPickerWidget->SetSelectedTextureName(Item.IsValid() ? *Item : FString());
	}

	if (Item.IsValid() && OnTextureSelectedDelegate.IsBound())
	{
		OnTextureSelectedDelegate.Execute(*Item);
	}
}

void SFrameRenderInspectorUI::OnTextureFilterChanged(const FText& InFilterText)
{
	SearchText = InFilterText.ToString();
	RebuildFilteredOptions();
	ApplyFilteredTextureSelection();

	if (TextureComboBox.IsValid())
	{
		TextureComboBox->RefreshOptions();
		TextureComboBox->SetSelectedItem(SelectedTextureOption);
	}
}

void SFrameRenderInspectorUI::OnOverlayOpacitySliderChanged(float NewValue)
{
	OverlayOpacity = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (OnOverlayOpacityChangedDelegate.IsBound())
	{
		OnOverlayOpacityChangedDelegate.Execute(OverlayOpacity);
	}
}

void SFrameRenderInspectorUI::OnOverlayCoverageSliderChanged(float NewValue)
{
	OverlayCoverage = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (OnOverlayCoverageChangedDelegate.IsBound())
	{
		OnOverlayCoverageChangedDelegate.Execute(OverlayCoverage);
	}
}

FReply SFrameRenderInspectorUI::OnComputeVisibleRangeClicked()
{
	if (OnComputeVisibleRangeDelegate.IsBound())
	{
		OnComputeVisibleRangeDelegate.Execute();
	}

	return FReply::Handled();
}

void SFrameRenderInspectorUI::OnRangeLockCheckStateChanged(ECheckBoxState NewState)
{
	bRangeLocked = (NewState == ECheckBoxState::Checked);

	if (OnRangeLockChangedDelegate.IsBound())
	{
		OnRangeLockChangedDelegate.Execute(bRangeLocked);
	}
}

void SFrameRenderInspectorUI::OnRangeMinTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	double ParsedValue = 0.0;
	if (LexTryParseString(ParsedValue, *NewText.ToString()))
	{
		RangeMin = static_cast<float>(ParsedValue);
		bHasRange = true;
		BroadcastRangeEdited();
	}
}

void SFrameRenderInspectorUI::OnRangeMaxTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	double ParsedValue = 0.0;
	if (LexTryParseString(ParsedValue, *NewText.ToString()))
	{
		RangeMax = static_cast<float>(ParsedValue);
		bHasRange = true;
		BroadcastRangeEdited();
	}
}

void SFrameRenderInspectorUI::BroadcastRangeEdited()
{
	if (OnRangeEditedDelegate.IsBound())
	{
		OnRangeEditedDelegate.Execute(RangeMin, RangeMax);
	}
}
