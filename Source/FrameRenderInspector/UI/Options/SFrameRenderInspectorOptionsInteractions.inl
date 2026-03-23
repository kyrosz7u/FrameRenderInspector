void SFrameRenderInspectorUI::SetOnRangeLockChanged(FOnRangeLockChanged InOnRangeLockChanged)
{
	OnRangeLockChangedDelegate = InOnRangeLockChanged;
}

void SFrameRenderInspectorUI::SetOnRangeEdited(FOnRangeEdited InOnRangeEdited)
{
	OnRangeEditedDelegate = InOnRangeEdited;
}

void SFrameRenderInspectorUI::OnRenderOptionFilterChanged(const FText& InFilterText)
{
	RenderOptionFilterText = InFilterText.ToString();
	RebuildFilteredRenderOptions();
}

FReply SFrameRenderInspectorUI::OnPreviousRenderOptionsPageClicked()
{
	CurrentRenderOptionsPage = FMath::Max(CurrentRenderOptionsPage - 1, 0);
	RebuildVisibleRenderOptions();
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnNextRenderOptionsPageClicked()
{
	CurrentRenderOptionsPage = FMath::Min(CurrentRenderOptionsPage + 1, GetRenderOptionsPageCount() - 1);
	RebuildVisibleRenderOptions();
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnModeButtonClicked(EInspectorMode NewMode)
{
	SetInspectorMode(NewMode);
	return FReply::Handled();
}
