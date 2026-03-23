#include "SFrameRenderInspectorUI.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Misc/DefaultValueHelper.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	const FName BufferMonoTextStyle(TEXT("NormalText"));

	FString FormatBinary(uint32 Value)
	{
		FString Result;
		Result.Reserve(35);
		for (int32 BitIndex = 31; BitIndex >= 0; --BitIndex)
		{
			Result.AppendChar(((Value >> BitIndex) & 1u) != 0u ? TEXT('1') : TEXT('0'));
			if (BitIndex > 0 && BitIndex % 8 == 0)
			{
				Result.AppendChar(TEXT(' '));
			}
		}
		return Result;
	}

	template <typename NumericType>
	bool TryParseIntegerLiteral(const FString& InText, NumericType& OutValue)
	{
		const FString Trimmed = InText.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}

		if (Trimmed.StartsWith(TEXT("0x")) || Trimmed.StartsWith(TEXT("0X")))
		{
			const uint64 ParsedValue = FParse::HexNumber64(*Trimmed);
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}

		if constexpr (TIsSigned<NumericType>::Value)
		{
			int64 ParsedValue = 0;
			if (!LexTryParseString(ParsedValue, *Trimmed))
			{
				return false;
			}
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}
		else
		{
			uint64 ParsedValue = 0;
			if (!LexTryParseString(ParsedValue, *Trimmed))
			{
				return false;
			}
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}
	}
}

void SFrameRenderInspectorUI::Construct(const FArguments& InArgs)
{
	FSlateFontInfo SectionTitleFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	SectionTitleFont.Size += 6;

	FSlateFontInfo ControlFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	ControlFont.Size += 2;

	BufferFormatOptions =
	{
		MakeShared<FString>(TEXT("Float")),
		MakeShared<FString>(TEXT("Int")),
		MakeShared<FString>(TEXT("UInt")),
		MakeShared<FString>(TEXT("Hex"))
	};
	SelectedBufferFormatOption = BufferFormatOptions[2];
	BufferStatusMessage = TEXT("Click Refresh to read back the selected buffer.");

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Textures")))
					.Font(SectionTitleFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(0.0f, 3.0f))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Texture Name Filter")))
						.Font(ControlFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.65f, 1.0f)))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SSearchBox)
						.OnTextChanged(this, &SFrameRenderInspectorUI::OnTextureFilterChanged)
					]
					+ SUniformGridPanel::Slot(0, 1)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Texture Name")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 1)
					[
						SAssignNew(TextureComboBox, SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&FilteredTextureOptions)
						.OnGenerateWidget(this, &SFrameRenderInspectorUI::GenerateTextureOptionWidget)
						.OnSelectionChanged(this, &SFrameRenderInspectorUI::OnTextureSelectionChanged)
						[
							SNew(STextBlock)
							.Text(this, &SFrameRenderInspectorUI::GetSelectedTextureText)
							.Font(ControlFont)
						]
					]
					+ SUniformGridPanel::Slot(0, 2)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Overlay Opacity")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 2)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSlider)
							.Value(OverlayOpacity)
							.OnValueChanged(this, &SFrameRenderInspectorUI::OnOverlayOpacitySliderChanged)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SFrameRenderInspectorUI::GetOverlayOpacityText)
							.Font(ControlFont)
						]
					]
					+ SUniformGridPanel::Slot(0, 3)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Overlay Coverage")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 3)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSlider)
							.Value(OverlayCoverage)
							.OnValueChanged(this, &SFrameRenderInspectorUI::OnOverlayCoverageSliderChanged)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SFrameRenderInspectorUI::GetOverlayCoverageText)
							.Font(ControlFont)
						]
					]
					+ SUniformGridPanel::Slot(0, 4)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Range")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Min")))
								.Font(ControlFont)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[
								SNew(SEditableTextBox)
								.Text(this, &SFrameRenderInspectorUI::GetRangeMinText)
								.OnTextCommitted(this, &SFrameRenderInspectorUI::OnRangeMinTextCommitted)
								.Font(ControlFont)
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Max")))
								.Font(ControlFont)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SNew(SEditableTextBox)
								.Text(this, &SFrameRenderInspectorUI::GetRangeMaxText)
								.OnTextCommitted(this, &SFrameRenderInspectorUI::OnRangeMaxTextCommitted)
								.Font(ControlFont)
							]
						]
					]
					+ SUniformGridPanel::Slot(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Visible Range")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 5)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 10.0f, 0.0f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Compute Visible Range")))
							.OnClicked(this, &SFrameRenderInspectorUI::OnComputeVisibleRangeClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SFrameRenderInspectorUI::GetRangeLockCheckState)
							.OnCheckStateChanged(this, &SFrameRenderInspectorUI::OnRangeLockCheckStateChanged)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Lock Range")))
								.Font(ControlFont)
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 14.0f, 0.0f, 14.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Buffers")))
					.Font(SectionTitleFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.45f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(0.0f, 3.0f))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Filter Buffers")))
						.Font(ControlFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.75f, 0.3f)))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SSearchBox)
						.OnTextChanged(this, &SFrameRenderInspectorUI::OnBufferFilterChanged)
					]
					+ SUniformGridPanel::Slot(0, 1)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Buffer")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 1)
					[
						SAssignNew(BufferComboBox, SComboBox<TSharedPtr<FBufferDebuggerItem>>)
						.OptionsSource(&FilteredBufferOptions)
						.OnGenerateWidget(this, &SFrameRenderInspectorUI::GenerateBufferOptionWidget)
						.OnSelectionChanged(this, &SFrameRenderInspectorUI::OnBufferSelectionChanged)
						[
							SNew(STextBlock)
							.Text(this, &SFrameRenderInspectorUI::GetSelectedBufferText)
							.Font(ControlFont)
						]
					]
					+ SUniformGridPanel::Slot(0, 2)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Stride")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 2)
					[
						SNew(STextBlock)
						.Text(this, &SFrameRenderInspectorUI::GetBufferStrideText)
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(0, 3)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Count")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 3)
					[
						SNew(STextBlock)
						.Text(this, &SFrameRenderInspectorUI::GetBufferCountText)
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(0, 4)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Rows")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &SFrameRenderInspectorUI::GetRowsText)
							.OnTextCommitted(this, &SFrameRenderInspectorUI::OnRowsTextCommitted)
							.Font(ControlFont)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(6.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Columns")))
							.Font(ControlFont)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &SFrameRenderInspectorUI::GetColumnsText)
							.OnTextCommitted(this, &SFrameRenderInspectorUI::OnColumnsTextCommitted)
							.Font(ControlFont)
						]
					]
					+ SUniformGridPanel::Slot(0, 5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Display Format")))
						.Font(ControlFont)
					]
					+ SUniformGridPanel::Slot(1, 5)
					[
						SAssignNew(BufferFormatComboBox, SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&BufferFormatOptions)
						.OnGenerateWidget(this, &SFrameRenderInspectorUI::GenerateBufferFormatOptionWidget)
						.OnSelectionChanged(this, &SFrameRenderInspectorUI::OnBufferFormatSelectionChanged)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() -> FText
							{
								return SelectedBufferFormatOption.IsValid() ? FText::FromString(*SelectedBufferFormatOption) : FText::FromString(TEXT("UInt"));
							})
							.Font(ControlFont)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("Copy"))).OnClicked(this, &SFrameRenderInspectorUI::OnCopyBufferPageClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("<"))).OnClicked(this, &SFrameRenderInspectorUI::OnPreviousBufferPageClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(STextBlock).Text(this, &SFrameRenderInspectorUI::GetBufferPageText).Font(ControlFont)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT(">"))).OnClicked(this, &SFrameRenderInspectorUI::OnNextBufferPageClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Jump To"))).Font(ControlFont)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(110.0f)
							[
								SNew(SEditableTextBox).OnTextChanged(this, &SFrameRenderInspectorUI::OnJumpAddressTextChanged).Font(ControlFont)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("Go"))).OnClicked(this, &SFrameRenderInspectorUI::OnGoToBufferAddressClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked(this, &SFrameRenderInspectorUI::OnRefreshBufferClicked)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Search"))).Font(ControlFont)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(140.0f)
							[
								SNew(SEditableTextBox).OnTextChanged(this, &SFrameRenderInspectorUI::OnSearchValueTextChanged).Font(ControlFont)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SButton).Text(FText::FromString(TEXT("Find"))).OnClicked(this, &SFrameRenderInspectorUI::OnSearchBufferClicked)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(FText::FromString(TEXT("Next Match"))).OnClicked(this, &SFrameRenderInspectorUI::OnNextSearchMatchClicked)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(this, &SFrameRenderInspectorUI::GetBufferStatusText)
					.Font(ControlFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.75f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(BufferRowsView, SListView<TSharedPtr<FBufferRowEntry>>)
					.ListItemsSource(&VisibleBufferRows)
					.OnGenerateRow(this, &SFrameRenderInspectorUI::OnGenerateBufferRowWidget)
					.SelectionMode(ESelectionMode::None)
					.ItemHeight(24.0f)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 14.0f, 0.0f, 14.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Render Options")))
					.Font(SectionTitleFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.92f, 0.78f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(0.0f, 3.0f))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Filter Options")))
						.Font(ControlFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.92f, 0.65f)))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SSearchBox)
							.OnTextChanged(this, &SFrameRenderInspectorUI::OnRenderOptionFilterChanged)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("<")))
							.OnClicked(this, &SFrameRenderInspectorUI::OnPreviousRenderOptionsPageClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(this, &SFrameRenderInspectorUI::GetRenderOptionsPageText)
							.Font(ControlFont)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT(">")))
							.OnClicked(this, &SFrameRenderInspectorUI::OnNextRenderOptionsPageClicked)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SAssignNew(RenderOptionsView, SListView<TSharedPtr<FRenderOptionItem>>)
					.ListItemsSource(&VisibleRenderOptions)
					.OnGenerateRow(this, &SFrameRenderInspectorUI::OnGenerateRenderOptionRowWidget)
					.SelectionMode(ESelectionMode::None)
					.ItemHeight(28.0f)
				]
			]
		]
	];
}

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

	if (!SelectedTextureOption.IsValid() && FilteredTextureOptions.Num() > 0)
	{
		SelectedTextureOption = FilteredTextureOptions[0];
	}

	if (TextureComboBox.IsValid())
	{
		TextureComboBox->RefreshOptions();
		TextureComboBox->SetSelectedItem(SelectedTextureOption);
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

	if (!SelectedBufferOption.IsValid() && FilteredBufferOptions.Num() > 0)
	{
		SelectedBufferOption = FilteredBufferOptions[0];
	}

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

void SFrameRenderInspectorUI::RebuildFilteredOptions()
{
	FilteredTextureOptions.Empty();

	for (const TSharedPtr<FString>& Option : AllTextureOptions)
	{
		if (!Option.IsValid())
		{
			continue;
		}

		if (SearchText.IsEmpty() || Option->Contains(SearchText))
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

		if (BufferFilterText.IsEmpty() || Option->Name.Contains(BufferFilterText))
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

		if (RenderOptionFilterText.IsEmpty() || Option->Name.Contains(RenderOptionFilterText))
		{
			FilteredRenderOptions.Add(Option);
		}
	}

	CurrentRenderOptionsPage = 0;
	RebuildVisibleRenderOptions();
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

void SFrameRenderInspectorUI::OnTextureSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	SelectedTextureOption = Item;

	if (Item.IsValid() && OnTextureSelectedDelegate.IsBound())
	{
		OnTextureSelectedDelegate.Execute(*Item);
	}
}

void SFrameRenderInspectorUI::OnTextureFilterChanged(const FText& InFilterText)
{
	SearchText = InFilterText.ToString();
	RebuildFilteredOptions();

	if (!SelectedTextureOption.IsValid() && FilteredTextureOptions.Num() > 0)
	{
		SelectedTextureOption = FilteredTextureOptions[0];
	}

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

	if (!SelectedBufferOption.IsValid() && FilteredBufferOptions.Num() > 0)
	{
		SelectedBufferOption = FilteredBufferOptions[0];
	}

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
