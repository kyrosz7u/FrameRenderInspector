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
				.Padding(0.0f, 0.0f, 0.0f, 14.0f)
				[
					SNew(SBorder)
					.Padding(14.0f)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Frame Render Inspector")))
							.Font(SectionTitleFont)
							.ColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.96f, 1.0f)))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Inspect textures, buffers, and render options with fast mode-based filtering.")))
							.Font(ControlFont)
							.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.82f, 0.9f)))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Mode")))
									.Font(ControlFont)
									.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("All")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::All)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::All)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("General")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::General)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::General)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Lumen")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::Lumen)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::Lumen)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Nanite")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::Nanite)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::Nanite)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(54.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("PostProcess")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::PostProcess)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::PostProcess)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Shadow")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::Shadow)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::Shadow)
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Virtual Shadow Map")))
									.ButtonColorAndOpacity(this, &SFrameRenderInspectorUI::GetModeButtonColor, EInspectorMode::VirtualShadowMap)
									.OnClicked(this, &SFrameRenderInspectorUI::OnModeButtonClicked, EInspectorMode::VirtualShadowMap)
								]
							]
						]
					]
				]
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

