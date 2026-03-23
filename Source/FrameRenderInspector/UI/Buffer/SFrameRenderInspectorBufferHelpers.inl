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

FText SFrameRenderInspectorUI::GetSelectedBufferText() const
{
	return SelectedBufferOption.IsValid() ? FText::FromString(SelectedBufferOption->Name) : FText::FromString(TEXT("Select Buffer"));
}

FText SFrameRenderInspectorUI::GetBufferStrideText() const
{
	return FText::FromString(BufferStride > 0 ? FString::Printf(TEXT("%u"), BufferStride) : TEXT("N/A"));
}

FText SFrameRenderInspectorUI::GetBufferCountText() const
{
	return FText::FromString(BufferCount > 0 ? FString::Printf(TEXT("%u"), BufferCount) : TEXT("N/A"));
}

FText SFrameRenderInspectorUI::GetBufferPageText() const
{
	const int32 PageCount = GetBufferPageCount();
	const int32 CurrentPage = PageCount > 0 ? CurrentBufferPage + 1 : 0;
	return FText::FromString(FString::Printf(TEXT("Page %d/%d"), CurrentPage, PageCount));
}

FText SFrameRenderInspectorUI::GetBufferStatusText() const
{
	return FText::FromString(BufferStatusMessage);
}

FText SFrameRenderInspectorUI::GetRowsText() const
{
	return FText::AsNumber(BufferRows);
}

FText SFrameRenderInspectorUI::GetColumnsText() const
{
	return FText::AsNumber(BufferColumns);
}

int32 SFrameRenderInspectorUI::GetBufferWordCount() const
{
	return BufferData.Num() / static_cast<int32>(sizeof(uint32));
}

int32 SFrameRenderInspectorUI::GetBufferPageCount() const
{
	const int32 CellsPerPage = GetBufferCellsPerPage();
	return CellsPerPage > 0 ? FMath::Max(1, FMath::DivideAndRoundUp(GetBufferWordCount(), CellsPerPage)) : 1;
}

int32 SFrameRenderInspectorUI::GetBufferCellsPerPage() const
{
	return FMath::Max(BufferRows, 1) * FMath::Max(BufferColumns, 1);
}

void SFrameRenderInspectorUI::ClampBufferPage()
{
	CurrentBufferPage = FMath::Clamp(CurrentBufferPage, 0, GetBufferPageCount() - 1);
}

FString SFrameRenderInspectorUI::FormatBufferValue(int32 ElementIndex) const
{
	uint32 RawValue = 0;
	if (!TryGetBufferWord(ElementIndex, RawValue))
	{
		return TEXT("");
	}

	switch (BufferDisplayFormat)
	{
	case EBufferDisplayFormat::Float:
		{
			float FloatValue = 0.0f;
			FMemory::Memcpy(&FloatValue, &RawValue, sizeof(float));
			return FString::Printf(TEXT("%g"), FloatValue);
		}
	case EBufferDisplayFormat::Int:
		return FString::Printf(TEXT("%d"), static_cast<int32>(RawValue));
	case EBufferDisplayFormat::Hex:
		return FString::Printf(TEXT("%08X"), RawValue);
	case EBufferDisplayFormat::UInt:
	default:
		return FString::Printf(TEXT("%u"), RawValue);
	}
}

FString SFrameRenderInspectorUI::BuildBufferValueTooltip(int32 ElementIndex) const
{
	uint32 RawValue = 0;
	if (!TryGetBufferWord(ElementIndex, RawValue))
	{
		return TEXT("");
	}

	float FloatValue = 0.0f;
	FMemory::Memcpy(&FloatValue, &RawValue, sizeof(float));

	return FString::Printf(
		TEXT("Addr: 0x%08X\nFloat: %g\nInt32: %d\nUInt32: %u\nHex: %08X\nBinary: %s"),
		ElementIndex * 4,
		FloatValue,
		static_cast<int32>(RawValue),
		RawValue,
		RawValue,
		*FormatBinary(RawValue));
}

FText SFrameRenderInspectorUI::GetBufferValueText(int32 ElementIndex) const
{
	return FText::FromString(FormatBufferValue(ElementIndex));
}

FLinearColor SFrameRenderInspectorUI::GetBufferValueColor(int32 ElementIndex) const
{
	return SearchMatchIndices.Contains(ElementIndex) ? FLinearColor(1.0f, 0.87f, 0.25f) : FLinearColor(0.2f, 0.9f, 0.25f);
}

bool SFrameRenderInspectorUI::TryGetBufferWord(int32 ElementIndex, uint32& OutValue) const
{
	const int32 ByteOffset = ElementIndex * static_cast<int32>(sizeof(uint32));
	if (!BufferData.IsValidIndex(ByteOffset + 3))
	{
		return false;
	}

	FMemory::Memcpy(&OutValue, BufferData.GetData() + ByteOffset, sizeof(uint32));
	return true;
}

bool SFrameRenderInspectorUI::TryParseAddress(const FString& InText, uint32& OutAddress) const
{
	return TryParseIntegerLiteral(InText, OutAddress);
}

bool SFrameRenderInspectorUI::TryParseSearchWord(const FString& InText, uint32& OutValue) const
{
	switch (BufferDisplayFormat)
	{
	case EBufferDisplayFormat::Float:
		{
			float FloatValue = 0.0f;
			if (!FDefaultValueHelper::ParseFloat(InText, FloatValue))
			{
				return false;
			}

			FMemory::Memcpy(&OutValue, &FloatValue, sizeof(uint32));
			return true;
		}
	case EBufferDisplayFormat::Int:
		{
			int32 IntValue = 0;
			if (!TryParseIntegerLiteral(InText, IntValue))
			{
				return false;
			}

			OutValue = static_cast<uint32>(IntValue);
			return true;
		}
	case EBufferDisplayFormat::Hex:
	case EBufferDisplayFormat::UInt:
	default:
		return TryParseIntegerLiteral(InText, OutValue);
	}
}

void SFrameRenderInspectorUI::RebuildSearchMatches()
{
	SearchMatchIndices.Empty();
	CurrentSearchMatchCursor = INDEX_NONE;

	uint32 TargetValue = 0;
	if (!TryParseSearchWord(SearchValueText, TargetValue))
	{
		return;
	}

	for (int32 ElementIndex = 0; ElementIndex < GetBufferWordCount(); ++ElementIndex)
	{
		uint32 WordValue = 0;
		if (TryGetBufferWord(ElementIndex, WordValue) && WordValue == TargetValue)
		{
			SearchMatchIndices.Add(ElementIndex);
		}
	}
}

void SFrameRenderInspectorUI::JumpToElementIndex(int32 ElementIndex)
{
	const int32 CellsPerPage = GetBufferCellsPerPage();
	if (CellsPerPage <= 0)
	{
		return;
	}

	const int32 ClampedElement = FMath::Clamp(ElementIndex, 0, FMath::Max(0, GetBufferWordCount() - 1));
	CurrentBufferPage = ClampedElement / CellsPerPage;
	ClampBufferPage();
	RebuildBufferRows();
}
