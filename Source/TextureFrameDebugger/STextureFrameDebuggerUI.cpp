#include "STextureFrameDebuggerUI.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

void STextureFrameDebuggerUI::Construct(const FArguments& InArgs)
{
	FSlateFontInfo SectionTitleFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	SectionTitleFont.Size += 6;

	FSlateFontInfo ControlFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	ControlFont.Size += 2;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Texture Visualizer")))
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
					.OnTextChanged(this, &STextureFrameDebuggerUI::OnTextureFilterChanged)
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
					.OnGenerateWidget(this, &STextureFrameDebuggerUI::GenerateTextureOptionWidget)
					.OnSelectionChanged(this, &STextureFrameDebuggerUI::OnTextureSelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &STextureFrameDebuggerUI::GetSelectedTextureText)
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
						.OnValueChanged(this, &STextureFrameDebuggerUI::OnOverlayOpacitySliderChanged)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &STextureFrameDebuggerUI::GetOverlayOpacityText)
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
						.OnValueChanged(this, &STextureFrameDebuggerUI::OnOverlayCoverageSliderChanged)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &STextureFrameDebuggerUI::GetOverlayCoverageText)
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
							.Text(this, &STextureFrameDebuggerUI::GetRangeMinText)
							.OnTextCommitted(this, &STextureFrameDebuggerUI::OnRangeMinTextCommitted)
							.Font(ControlFont)
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 0.0f, 0.0f)
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
							.Text(this, &STextureFrameDebuggerUI::GetRangeMaxText)
							.OnTextCommitted(this, &STextureFrameDebuggerUI::OnRangeMaxTextCommitted)
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
						.OnClicked(this, &STextureFrameDebuggerUI::OnComputeVisibleRangeClicked)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked(this, &STextureFrameDebuggerUI::GetRangeLockCheckState)
						.OnCheckStateChanged(this, &STextureFrameDebuggerUI::OnRangeLockCheckStateChanged)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Lock Range")))
							.Font(ControlFont)
						]
					]
				]
			]
		]
	];
}

void STextureFrameDebuggerUI::UpdateTextureOptions(const TArray<FString>& TextureNames, const FString& SelectedTextureName)
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

void STextureFrameDebuggerUI::SetOnTextureSelected(FOnTextureSelected InOnTextureSelected)
{
	OnTextureSelectedDelegate = InOnTextureSelected;
}

void STextureFrameDebuggerUI::SetOnOverlayOpacityChanged(FOnOverlayOpacityChanged InOnOverlayOpacityChanged)
{
	OnOverlayOpacityChangedDelegate = InOnOverlayOpacityChanged;
}

void STextureFrameDebuggerUI::SetOnOverlayCoverageChanged(FOnOverlayCoverageChanged InOnOverlayCoverageChanged)
{
	OnOverlayCoverageChangedDelegate = InOnOverlayCoverageChanged;
}

void STextureFrameDebuggerUI::SetOnComputeVisibleRange(FOnComputeVisibleRange InOnComputeVisibleRange)
{
	OnComputeVisibleRangeDelegate = InOnComputeVisibleRange;
}

void STextureFrameDebuggerUI::SetOnRangeLockChanged(FOnRangeLockChanged InOnRangeLockChanged)
{
	OnRangeLockChangedDelegate = InOnRangeLockChanged;
}

void STextureFrameDebuggerUI::SetOnRangeEdited(FOnRangeEdited InOnRangeEdited)
{
	OnRangeEditedDelegate = InOnRangeEdited;
}

void STextureFrameDebuggerUI::SetOverlaySettings(float InOpacity, float InCoverage)
{
	OverlayOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
	OverlayCoverage = FMath::Clamp(InCoverage, 0.0f, 1.0f);
}

void STextureFrameDebuggerUI::SetRangeState(float InMin, float InMax, bool bInHasRange, bool bInRangeLocked)
{
	RangeMin = InMin;
	RangeMax = InMax;
	bHasRange = bInHasRange;
	bRangeLocked = bInRangeLocked;
}

void STextureFrameDebuggerUI::RebuildFilteredOptions()
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

TSharedRef<SWidget> STextureFrameDebuggerUI::GenerateTextureOptionWidget(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"));
}

void STextureFrameDebuggerUI::OnTextureSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	SelectedTextureOption = Item;

	if (Item.IsValid() && OnTextureSelectedDelegate.IsBound())
	{
		OnTextureSelectedDelegate.Execute(*Item);
	}
}

void STextureFrameDebuggerUI::OnTextureFilterChanged(const FText& InFilterText)
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

FText STextureFrameDebuggerUI::GetSelectedTextureText() const
{
	return SelectedTextureOption.IsValid()
		? FText::FromString(*SelectedTextureOption)
		: FText::FromString(TEXT("Select Texture"));
}

void STextureFrameDebuggerUI::OnOverlayOpacitySliderChanged(float NewValue)
{
	OverlayOpacity = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (OnOverlayOpacityChangedDelegate.IsBound())
	{
		OnOverlayOpacityChangedDelegate.Execute(OverlayOpacity);
	}
}

void STextureFrameDebuggerUI::OnOverlayCoverageSliderChanged(float NewValue)
{
	OverlayCoverage = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (OnOverlayCoverageChangedDelegate.IsBound())
	{
		OnOverlayCoverageChangedDelegate.Execute(OverlayCoverage);
	}
}

FReply STextureFrameDebuggerUI::OnComputeVisibleRangeClicked()
{
	if (OnComputeVisibleRangeDelegate.IsBound())
	{
		OnComputeVisibleRangeDelegate.Execute();
	}

	return FReply::Handled();
}

void STextureFrameDebuggerUI::OnRangeLockCheckStateChanged(ECheckBoxState NewState)
{
	bRangeLocked = (NewState == ECheckBoxState::Checked);

	if (OnRangeLockChangedDelegate.IsBound())
	{
		OnRangeLockChangedDelegate.Execute(bRangeLocked);
	}
}

void STextureFrameDebuggerUI::OnRangeMinTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	double ParsedValue = 0.0;
	if (LexTryParseString(ParsedValue, *NewText.ToString()))
	{
		RangeMin = static_cast<float>(ParsedValue);
		bHasRange = true;
		BroadcastRangeEdited();
	}
}

void STextureFrameDebuggerUI::OnRangeMaxTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	double ParsedValue = 0.0;
	if (LexTryParseString(ParsedValue, *NewText.ToString()))
	{
		RangeMax = static_cast<float>(ParsedValue);
		bHasRange = true;
		BroadcastRangeEdited();
	}
}

void STextureFrameDebuggerUI::BroadcastRangeEdited()
{
	if (OnRangeEditedDelegate.IsBound())
	{
		OnRangeEditedDelegate.Execute(RangeMin, RangeMax);
	}
}

FText STextureFrameDebuggerUI::GetOverlayOpacityText() const
{
	return FText::FromString(FString::Printf(TEXT("%.2f"), OverlayOpacity));
}

FText STextureFrameDebuggerUI::GetOverlayCoverageText() const
{
	return FText::FromString(FString::Printf(TEXT("%.2f"), OverlayCoverage));
}

FText STextureFrameDebuggerUI::GetRangeMinText() const
{
	return bHasRange ? FText::FromString(FString::Printf(TEXT("%g"), RangeMin)) : FText::FromString(TEXT("N/A"));
}

FText STextureFrameDebuggerUI::GetRangeMaxText() const
{
	return bHasRange ? FText::FromString(FString::Printf(TEXT("%g"), RangeMax)) : FText::FromString(TEXT("N/A"));
}

ECheckBoxState STextureFrameDebuggerUI::GetRangeLockCheckState() const
{
	return bRangeLocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}
