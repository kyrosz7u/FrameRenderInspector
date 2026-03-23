TSharedRef<SWidget> SFrameRenderInspectorUI::GenerateTextureOptionWidget(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"));
}

FText SFrameRenderInspectorUI::GetSelectedTextureText() const
{
	return SelectedTextureOption.IsValid() ? FText::FromString(*SelectedTextureOption) : FText::FromString(TEXT("Select Texture"));
}

FText SFrameRenderInspectorUI::GetOverlayOpacityText() const
{
	return FText::FromString(FString::Printf(TEXT("%.2f"), OverlayOpacity));
}

FText SFrameRenderInspectorUI::GetOverlayCoverageText() const
{
	return FText::FromString(FString::Printf(TEXT("%.2f"), OverlayCoverage));
}

FText SFrameRenderInspectorUI::GetRangeMinText() const
{
	return bHasRange ? FText::FromString(FString::Printf(TEXT("%g"), RangeMin)) : FText::FromString(TEXT("N/A"));
}

FText SFrameRenderInspectorUI::GetRangeMaxText() const
{
	return bHasRange ? FText::FromString(FString::Printf(TEXT("%g"), RangeMax)) : FText::FromString(TEXT("N/A"));
}

ECheckBoxState SFrameRenderInspectorUI::GetRangeLockCheckState() const
{
	return bRangeLocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
