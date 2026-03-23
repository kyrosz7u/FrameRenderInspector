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

void SFrameRenderInspectorUI::UpdateRenderOptions(const TArray<FRenderOptionItem>& RenderOptions)
{
	AllRenderOptions.Empty();
	for (const FRenderOptionItem& RenderOption : RenderOptions)
	{
		AllRenderOptions.Add(MakeShared<FRenderOptionItem>(RenderOption));
	}

	RebuildFilteredRenderOptions();
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

void SFrameRenderInspectorUI::SetInspectorModeValue(int32 InModeValue)
{
	const int32 ClampedModeValue = FMath::Clamp(InModeValue, 0, static_cast<int32>(EInspectorMode::VirtualShadowMap));
	SetInspectorMode(static_cast<EInspectorMode>(ClampedModeValue));
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

void SFrameRenderInspectorUI::SetOnTextureSelected(FOnTextureSelected InOnTextureSelected)
{
	OnTextureSelectedDelegate = InOnTextureSelected;
}

void SFrameRenderInspectorUI::SetOnBufferSelected(FOnBufferSelected InOnBufferSelected)
{
	OnBufferSelectedDelegate = InOnBufferSelected;
}

void SFrameRenderInspectorUI::SetOnRefreshBuffer(FOnRefreshBuffer InOnRefreshBuffer)
{
	OnRefreshBufferDelegate = InOnRefreshBuffer;
}

void SFrameRenderInspectorUI::SetOnRenderOptionBoolChanged(FOnRenderOptionBoolChanged InOnRenderOptionBoolChanged)
{
	OnRenderOptionBoolChangedDelegate = InOnRenderOptionBoolChanged;
}

void SFrameRenderInspectorUI::SetOnRenderOptionValueCommitted(FOnRenderOptionValueCommitted InOnRenderOptionValueCommitted)
{
	OnRenderOptionValueCommittedDelegate = InOnRenderOptionValueCommitted;
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

void SFrameRenderInspectorUI::SetOnRangeLockChanged(FOnRangeLockChanged InOnRangeLockChanged)
{
	OnRangeLockChangedDelegate = InOnRangeLockChanged;
}

void SFrameRenderInspectorUI::SetOnRangeEdited(FOnRangeEdited InOnRangeEdited)
{
	OnRangeEditedDelegate = InOnRangeEdited;
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

int32 SFrameRenderInspectorUI::GetInspectorModeValue() const
{
	return static_cast<int32>(ActiveMode);
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

void SFrameRenderInspectorUI::ApplyFilteredTextureSelection()
{
	if (!SelectedTextureOption.IsValid() && FilteredTextureOptions.Num() > 0)
	{
		SelectedTextureOption = FilteredTextureOptions[0];
	}
}

void SFrameRenderInspectorUI::ApplyFilteredBufferSelection()
{
	if (!SelectedBufferOption.IsValid() && FilteredBufferOptions.Num() > 0)
	{
		SelectedBufferOption = FilteredBufferOptions[0];
	}
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

FReply SFrameRenderInspectorUI::OnModeButtonClicked(EInspectorMode NewMode)
{
	SetInspectorMode(NewMode);
	return FReply::Handled();
}

FSlateColor SFrameRenderInspectorUI::GetModeButtonColor(EInspectorMode Mode) const
{
	if (Mode == ActiveMode)
	{
		return FSlateColor(FLinearColor(0.23f, 0.48f, 0.86f));
	}

	return FSlateColor(FLinearColor(0.16f, 0.18f, 0.22f));
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

TSharedRef<SWidget> SFrameRenderInspectorUI::GenerateTextureOptionWidget(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"));
}

TSharedRef<SWidget> SFrameRenderInspectorUI::GenerateBufferOptionWidget(TSharedPtr<FBufferDebuggerItem> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(Item->Name) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"));
}

TSharedRef<SWidget> SFrameRenderInspectorUI::GenerateBufferFormatOptionWidget(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"));
}

TSharedRef<ITableRow> SFrameRenderInspectorUI::OnGenerateBufferRowWidget(TSharedPtr<FBufferRowEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

	RowBox->AddSlot()
	.AutoWidth()
	.Padding(2.0f, 1.0f, 12.0f, 1.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("%08X"), Item.IsValid() ? Item->Address : 0u)))
		.Font(FAppStyle::GetFontStyle(BufferMonoTextStyle))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.78f, 0.78f)))
	];

	if (Item.IsValid())
	{
		for (int32 ColumnIndex = 0; ColumnIndex < BufferColumns; ++ColumnIndex)
		{
			RowBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f, 1.0f)
			[
				SNew(SBox)
				.MinDesiredWidth(BufferDisplayFormat == EBufferDisplayFormat::Hex ? 80.0f : 92.0f)
				[
					SNew(STextBlock)
					.Text(ColumnIndex < Item->ElementIndices.Num() ? GetBufferValueText(Item->ElementIndices[ColumnIndex]) : FText::FromString(TEXT("")))
					.Font(FAppStyle::GetFontStyle(BufferMonoTextStyle))
					.ColorAndOpacity(ColumnIndex < Item->ElementIndices.Num() ? FSlateColor(GetBufferValueColor(Item->ElementIndices[ColumnIndex])) : FSlateColor(FLinearColor::Transparent))
					.ToolTipText(ColumnIndex < Item->ElementIndices.Num() ? FText::FromString(BuildBufferValueTooltip(Item->ElementIndices[ColumnIndex])) : FText::GetEmpty())
				]
			];
		}
	}

	return SNew(STableRow<TSharedPtr<FBufferRowEntry>>, OwnerTable)[RowBox];
}

TSharedRef<ITableRow> SFrameRenderInspectorUI::OnGenerateRenderOptionRowWidget(TSharedPtr<FRenderOptionItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

	RowBox->AddSlot()
	.FillWidth(0.65f)
	.Padding(2.0f, 2.0f, 10.0f, 2.0f)
	[
		SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(Item->Name) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
	];

	if (Item.IsValid() && Item->ValueType == ERenderOptionValueType::Bool)
	{
		RowBox->AddSlot()
		.FillWidth(0.35f)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([Item]()
			{
				return Item->bBoolValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this, Item](ECheckBoxState NewState)
			{
				if (!Item.IsValid())
				{
					return;
				}

				Item->bBoolValue = (NewState == ECheckBoxState::Checked);
				Item->ValueText = Item->bBoolValue ? TEXT("1") : TEXT("0");
				if (OnRenderOptionBoolChangedDelegate.IsBound())
				{
					OnRenderOptionBoolChangedDelegate.Execute(Item->Name, Item->bBoolValue);
				}
			})
		];
	}
	else
	{
		RowBox->AddSlot()
		.FillWidth(0.35f)
		[
			SNew(SEditableTextBox)
			.Text_Lambda([Item]()
			{
				return Item.IsValid() ? FText::FromString(Item->ValueText) : FText::GetEmpty();
			})
			.OnTextCommitted_Lambda([this, Item](const FText& NewText, ETextCommit::Type CommitType)
			{
				if (!Item.IsValid())
				{
					return;
				}

				Item->ValueText = NewText.ToString();
				if (OnRenderOptionValueCommittedDelegate.IsBound())
				{
					OnRenderOptionValueCommittedDelegate.Execute(Item->Name, Item->ValueText);
				}
			})
			.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
		];
	}

	return SNew(STableRow<TSharedPtr<FRenderOptionItem>>, OwnerTable)[RowBox];
}

