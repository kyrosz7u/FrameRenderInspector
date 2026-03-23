FText SFrameRenderInspectorUI::GetSelectedTextureText() const
{
	return SelectedTextureOption.IsValid() ? FText::FromString(*SelectedTextureOption) : FText::FromString(TEXT("Select Texture"));
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

FText SFrameRenderInspectorUI::GetRenderOptionsPageText() const
{
	const int32 PageCount = GetRenderOptionsPageCount();
	const int32 CurrentPage = PageCount > 0 ? CurrentRenderOptionsPage + 1 : 0;
	return FText::FromString(FString::Printf(TEXT("Page %d/%d"), CurrentPage, PageCount));
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

int32 SFrameRenderInspectorUI::GetRenderOptionsPageCount() const
{
	return RenderOptionsPerPage > 0 ? FMath::Max(1, FMath::DivideAndRoundUp(FilteredRenderOptions.Num(), RenderOptionsPerPage)) : 1;
}

void SFrameRenderInspectorUI::ClampRenderOptionsPage()
{
	CurrentRenderOptionsPage = FMath::Clamp(CurrentRenderOptionsPage, 0, GetRenderOptionsPageCount() - 1);
}
