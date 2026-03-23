TSharedRef<SWidget> SFrameRenderInspectorUI::BuildBufferSection()
{
	const FSlateFontInfo SectionTitleFont = MakeSectionTitleFont();
	const FSlateFontInfo ControlFont = MakeControlFont();

	return SNew(SVerticalBox)
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
		];
}
