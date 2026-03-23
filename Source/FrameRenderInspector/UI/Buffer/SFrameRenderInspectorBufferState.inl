void SFrameRenderInspectorUI::UpdateBufferOptions(const TArray<FBufferDebuggerItem>& BufferItems, const FString& SelectedBufferName)
{
	AllBufferOptions.Empty();
	for (const FBufferDebuggerItem& BufferItem : BufferItems)
	{
		AllBufferOptions.Add(MakeShared<FBufferDebuggerItem>(BufferItem));
	}

	SelectedBufferOption.Reset();
	for (const TSharedPtr<FBufferDebuggerItem>& Option : AllBufferOptions)
	{
		if (Option.IsValid() && Option->Name == SelectedBufferName)
		{
			SelectedBufferOption = Option;
			break;
		}
	}

	RebuildFilteredBufferOptions();
	ApplyFilteredBufferSelection();

	if (SelectedBufferOption.IsValid())
	{
		BufferStride = SelectedBufferOption->Stride;
		BufferCount = SelectedBufferOption->Count;
	}

	if (BufferComboBox.IsValid())
	{
		BufferComboBox->RefreshOptions();
		bIsSyncingBufferSelection = true;
		BufferComboBox->SetSelectedItem(SelectedBufferOption);
		bIsSyncingBufferSelection = false;
	}
}

void SFrameRenderInspectorUI::SetBufferReadbackResult(const FBufferReadbackResult& InReadbackResult)
{
	BufferStride = InReadbackResult.Stride;
	BufferCount = InReadbackResult.Count;
	BufferData = InReadbackResult.Data;
	BufferStatusMessage = InReadbackResult.StatusMessage;
	CurrentBufferPage = 0;
	CurrentSearchMatchCursor = INDEX_NONE;
	RebuildSearchMatches();
	RebuildBufferRows();
}

void SFrameRenderInspectorUI::SetBufferViewSettings(int32 InRows, int32 InColumns, const FString& InFormatName)
{
	BufferRows = FMath::Clamp(InRows, 1, 128);
	BufferColumns = FMath::Clamp(InColumns, 1, 64);

	TSharedPtr<FString> MatchedFormat;
	for (const TSharedPtr<FString>& FormatOption : BufferFormatOptions)
	{
		if (FormatOption.IsValid() && FormatOption->Equals(InFormatName, ESearchCase::IgnoreCase))
		{
			MatchedFormat = FormatOption;
			break;
		}
	}

	if (!MatchedFormat.IsValid() && BufferFormatOptions.Num() > 0)
	{
		MatchedFormat = BufferFormatOptions[0];
	}

	if (MatchedFormat.IsValid())
	{
		SelectedBufferFormatOption = MatchedFormat;
		if (*MatchedFormat == TEXT("Float"))
		{
			BufferDisplayFormat = EBufferDisplayFormat::Float;
		}
		else if (*MatchedFormat == TEXT("Int"))
		{
			BufferDisplayFormat = EBufferDisplayFormat::Int;
		}
		else if (*MatchedFormat == TEXT("Hex"))
		{
			BufferDisplayFormat = EBufferDisplayFormat::Hex;
		}
		else
		{
			BufferDisplayFormat = EBufferDisplayFormat::UInt;
		}
	}

	if (BufferFormatComboBox.IsValid())
	{
		BufferFormatComboBox->SetSelectedItem(SelectedBufferFormatOption);
	}

	RebuildSearchMatches();
	RebuildBufferRows();
}

void SFrameRenderInspectorUI::SetOnBufferSelected(FOnBufferSelected InOnBufferSelected)
{
	OnBufferSelectedDelegate = InOnBufferSelected;
}

void SFrameRenderInspectorUI::SetOnRefreshBuffer(FOnRefreshBuffer InOnRefreshBuffer)
{
	OnRefreshBufferDelegate = InOnRefreshBuffer;
}

int32 SFrameRenderInspectorUI::GetBufferRowsSetting() const
{
	return BufferRows;
}

int32 SFrameRenderInspectorUI::GetBufferColumnsSetting() const
{
	return BufferColumns;
}

FString SFrameRenderInspectorUI::GetBufferFormatName() const
{
	return SelectedBufferFormatOption.IsValid() ? *SelectedBufferFormatOption : FString(TEXT("UInt"));
}

void SFrameRenderInspectorUI::RebuildFilteredBufferOptions()
{
	FilteredBufferOptions.Empty();

	for (const TSharedPtr<FBufferDebuggerItem>& Option : AllBufferOptions)
	{
		if (!Option.IsValid())
		{
			continue;
		}

		if (MatchesActiveMode(Option->Name) && (BufferFilterText.IsEmpty() || Option->Name.Contains(BufferFilterText, ESearchCase::IgnoreCase)))
		{
			FilteredBufferOptions.Add(Option);
		}
	}

	if (SelectedBufferOption.IsValid() && !FilteredBufferOptions.Contains(SelectedBufferOption))
	{
		SelectedBufferOption.Reset();
	}
}

void SFrameRenderInspectorUI::RebuildBufferRows()
{
	VisibleBufferRows.Empty();
	ClampBufferPage();

	const int32 WordCount = GetBufferWordCount();
	const int32 CellsPerPage = GetBufferCellsPerPage();
	if (WordCount <= 0 || CellsPerPage <= 0)
	{
		if (BufferRowsView.IsValid())
		{
			BufferRowsView->RequestListRefresh();
		}
		return;
	}

	const int32 StartWordIndex = CurrentBufferPage * CellsPerPage;
	const int32 EndWordIndex = FMath::Min(StartWordIndex + CellsPerPage, WordCount);

	for (int32 RowIndex = 0; RowIndex < BufferRows; ++RowIndex)
	{
		const int32 RowStart = StartWordIndex + RowIndex * BufferColumns;
		if (RowStart >= EndWordIndex)
		{
			break;
		}

		TSharedPtr<FBufferRowEntry> RowEntry = MakeShared<FBufferRowEntry>();
		RowEntry->Address = static_cast<uint32>(RowStart * 4);
		for (int32 ColumnIndex = 0; ColumnIndex < BufferColumns; ++ColumnIndex)
		{
			const int32 ElementIndex = RowStart + ColumnIndex;
			if (ElementIndex >= EndWordIndex)
			{
				break;
			}
			RowEntry->ElementIndices.Add(ElementIndex);
		}

		VisibleBufferRows.Add(RowEntry);
	}

	if (BufferRowsView.IsValid())
	{
		BufferRowsView->RequestListRefresh();
	}
}

void SFrameRenderInspectorUI::ApplyFilteredBufferSelection()
{
	if (!SelectedBufferOption.IsValid() && FilteredBufferOptions.Num() > 0)
	{
		SelectedBufferOption = FilteredBufferOptions[0];
	}
}
