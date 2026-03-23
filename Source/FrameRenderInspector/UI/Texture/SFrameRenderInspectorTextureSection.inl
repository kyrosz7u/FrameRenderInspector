TSharedRef<SWidget> SFrameRenderInspectorUI::BuildTextureSection()
{
	const FSlateFontInfo SectionTitleFont = MakeSectionTitleFont();
	const FSlateFontInfo ControlFont = MakeControlFont();

	return SNew(SVerticalBox)
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
				.Text(FText::FromString(TEXT("Visualize In Viewport")))
				.Font(ControlFont)
			]
			+ SUniformGridPanel::Slot(1, 2)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SFrameRenderInspectorUI::GetVisualizeInViewportCheckState)
				.OnCheckStateChanged(this, &SFrameRenderInspectorUI::OnVisualizeInViewportCheckStateChanged)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Enable DebuggerRT Overlay")))
					.Font(ControlFont)
				]
			]
			+ SUniformGridPanel::Slot(0, 3)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Overlay Opacity")))
				.Font(ControlFont)
			]
			+ SUniformGridPanel::Slot(1, 3)
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
			+ SUniformGridPanel::Slot(0, 4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Overlay Coverage")))
				.Font(ControlFont)
			]
			+ SUniformGridPanel::Slot(1, 4)
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
			+ SUniformGridPanel::Slot(0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Range")))
				.Font(ControlFont)
			]
			+ SUniformGridPanel::Slot(1, 5)
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
			+ SUniformGridPanel::Slot(0, 6)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Visible Range")))
				.Font(ControlFont)
			]
			+ SUniformGridPanel::Slot(1, 6)
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
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.Padding(10.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Pixel Picker")))
					.Font(ControlFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.85f, 1.0f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TexturePixelPickerWidget, SFrameRenderInspectorPixelPicker)
				]
			]
		];
}
