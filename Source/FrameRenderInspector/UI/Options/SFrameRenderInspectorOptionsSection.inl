TSharedRef<SWidget> SFrameRenderInspectorUI::BuildModeSelector()
{
	const FSlateFontInfo ControlFont = MakeControlFont();

	return SNew(SVerticalBox)
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
		];
}

TSharedRef<SWidget> SFrameRenderInspectorUI::BuildRenderOptionsSection()
{
	const FSlateFontInfo SectionTitleFont = MakeSectionTitleFont();
	const FSlateFontInfo ControlFont = MakeControlFont();

	return SNew(SVerticalBox)
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
		];
}
