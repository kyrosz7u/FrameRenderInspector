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

void SFrameRenderInspectorUI::OnBufferSelectionChanged(TSharedPtr<FBufferDebuggerItem> Item, ESelectInfo::Type SelectInfo)
{
	SelectedBufferOption = Item;

	if (bIsSyncingBufferSelection)
	{
		if (Item.IsValid())
		{
			BufferStride = Item->Stride;
			BufferCount = Item->Count;
		}
		return;
	}

	BufferData.Empty();
	BufferStatusMessage = TEXT("Selection changed. Click Refresh to read back the buffer.");
	CurrentBufferPage = 0;
	SearchMatchIndices.Empty();
	CurrentSearchMatchCursor = INDEX_NONE;

	if (Item.IsValid())
	{
		BufferStride = Item->Stride;
		BufferCount = Item->Count;

		if (OnBufferSelectedDelegate.IsBound())
		{
			OnBufferSelectedDelegate.Execute(Item->Name);
		}
	}

	RebuildBufferRows();
}

void SFrameRenderInspectorUI::OnBufferFilterChanged(const FText& InFilterText)
{
	BufferFilterText = InFilterText.ToString();
	RebuildFilteredBufferOptions();
	ApplyFilteredBufferSelection();

	if (BufferComboBox.IsValid())
	{
		BufferComboBox->RefreshOptions();
		bIsSyncingBufferSelection = true;
		BufferComboBox->SetSelectedItem(SelectedBufferOption);
		bIsSyncingBufferSelection = false;
	}
}

void SFrameRenderInspectorUI::OnRenderOptionFilterChanged(const FText& InFilterText)
{
	RenderOptionFilterText = InFilterText.ToString();
	RebuildFilteredRenderOptions();
}

void SFrameRenderInspectorUI::OnBufferFormatSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	SelectedBufferFormatOption = Item;
	if (!Item.IsValid())
	{
		return;
	}

	if (*Item == TEXT("Float"))
	{
		BufferDisplayFormat = EBufferDisplayFormat::Float;
	}
	else if (*Item == TEXT("Int"))
	{
		BufferDisplayFormat = EBufferDisplayFormat::Int;
	}
	else if (*Item == TEXT("Hex"))
	{
		BufferDisplayFormat = EBufferDisplayFormat::Hex;
	}
	else
	{
		BufferDisplayFormat = EBufferDisplayFormat::UInt;
	}

	RebuildSearchMatches();
	RebuildBufferRows();
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

FReply SFrameRenderInspectorUI::OnRefreshBufferClicked()
{
	if (OnRefreshBufferDelegate.IsBound())
	{
		BufferStatusMessage = TEXT("Queued GPU readback for the selected buffer.");
		OnRefreshBufferDelegate.Execute();
	}

	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnCopyBufferPageClicked()
{
	if (BufferData.Num() == 0)
	{
		BufferStatusMessage = TEXT("Nothing to copy. Read back a buffer first.");
		return FReply::Handled();
	}

	FString ClipboardText;
	const int32 CellsPerPage = GetBufferCellsPerPage();
	const int32 StartWordIndex = CurrentBufferPage * CellsPerPage;
	const int32 EndWordIndex = FMath::Min(StartWordIndex + CellsPerPage, GetBufferWordCount());

	for (int32 RowIndex = 0; RowIndex < BufferRows; ++RowIndex)
	{
		const int32 RowStart = StartWordIndex + RowIndex * BufferColumns;
		if (RowStart >= EndWordIndex)
		{
			break;
		}

		ClipboardText += FString::Printf(TEXT("%08X"), RowStart * 4);
		for (int32 ColumnIndex = 0; ColumnIndex < BufferColumns; ++ColumnIndex)
		{
			const int32 ElementIndex = RowStart + ColumnIndex;
			if (ElementIndex >= EndWordIndex)
			{
				break;
			}

			ClipboardText += TEXT("\t");
			ClipboardText += FormatBufferValue(ElementIndex);
		}
		ClipboardText += LINE_TERMINATOR;
	}

	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
	BufferStatusMessage = TEXT("Copied current page to clipboard.");
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnPreviousBufferPageClicked()
{
	CurrentBufferPage = FMath::Max(CurrentBufferPage - 1, 0);
	RebuildBufferRows();
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnNextBufferPageClicked()
{
	CurrentBufferPage = FMath::Min(CurrentBufferPage + 1, GetBufferPageCount() - 1);
	RebuildBufferRows();
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnGoToBufferAddressClicked()
{
	uint32 Address = 0;
	if (!TryParseAddress(JumpAddressText, Address))
	{
		BufferStatusMessage = TEXT("Invalid address. Use decimal or 0x-prefixed hexadecimal.");
		return FReply::Handled();
	}

	JumpToElementIndex(static_cast<int32>(Address / 4));
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnSearchBufferClicked()
{
	RebuildSearchMatches();
	if (SearchMatchIndices.Num() == 0)
	{
		BufferStatusMessage = TEXT("No matches found for the current display format.");
		return FReply::Handled();
	}

	CurrentSearchMatchCursor = 0;
	JumpToElementIndex(SearchMatchIndices[CurrentSearchMatchCursor]);
	BufferStatusMessage = FString::Printf(TEXT("Found %d matches."), SearchMatchIndices.Num());
	return FReply::Handled();
}

FReply SFrameRenderInspectorUI::OnNextSearchMatchClicked()
{
	if (SearchMatchIndices.Num() == 0)
	{
		BufferStatusMessage = TEXT("No search matches. Run Find first.");
		return FReply::Handled();
	}

	CurrentSearchMatchCursor = (CurrentSearchMatchCursor + 1) % SearchMatchIndices.Num();
	JumpToElementIndex(SearchMatchIndices[CurrentSearchMatchCursor]);
	BufferStatusMessage = FString::Printf(TEXT("Match %d / %d"), CurrentSearchMatchCursor + 1, SearchMatchIndices.Num());
	return FReply::Handled();
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

void SFrameRenderInspectorUI::OnRowsTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	int32 ParsedValue = 0;
	if (TryParseIntegerLiteral(NewText.ToString(), ParsedValue))
	{
		BufferRows = FMath::Clamp(ParsedValue, 1, 128);
		ClampBufferPage();
		RebuildBufferRows();
	}
}

void SFrameRenderInspectorUI::OnColumnsTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	int32 ParsedValue = 0;
	if (TryParseIntegerLiteral(NewText.ToString(), ParsedValue))
	{
		BufferColumns = FMath::Clamp(ParsedValue, 1, 64);
		ClampBufferPage();
		RebuildBufferRows();
	}
}

void SFrameRenderInspectorUI::OnJumpAddressTextChanged(const FText& NewText)
{
	JumpAddressText = NewText.ToString();
}

void SFrameRenderInspectorUI::OnSearchValueTextChanged(const FText& NewText)
{
	SearchValueText = NewText.ToString();
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

