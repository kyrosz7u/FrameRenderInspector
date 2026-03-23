void SFrameRenderInspectorUI::Construct(const FArguments& InArgs)
{
	InitializeBufferFormatOptions();
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
					BuildHeaderSection()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildTextureSection()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 14.0f, 0.0f, 14.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildBufferSection()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 14.0f, 0.0f, 14.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildRenderOptionsSection()
				]
			]
		]
	];
}

void SFrameRenderInspectorUI::InitializeBufferFormatOptions()
{
	BufferFormatOptions =
	{
		MakeShared<FString>(TEXT("Float")),
		MakeShared<FString>(TEXT("Int")),
		MakeShared<FString>(TEXT("UInt")),
		MakeShared<FString>(TEXT("Hex"))
	};

	SelectedBufferFormatOption = BufferFormatOptions[2];
}

FSlateFontInfo SFrameRenderInspectorUI::MakeSectionTitleFont() const
{
	FSlateFontInfo SectionTitleFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	SectionTitleFont.Size += 6;
	return SectionTitleFont;
}

FSlateFontInfo SFrameRenderInspectorUI::MakeControlFont() const
{
	FSlateFontInfo ControlFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");
	ControlFont.Size += 2;
	return ControlFont;
}

TSharedRef<SWidget> SFrameRenderInspectorUI::BuildHeaderSection()
{
	const FSlateFontInfo SectionTitleFont = MakeSectionTitleFont();
	const FSlateFontInfo ControlFont = MakeControlFont();

	return SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Frame Render Inspector")))
					.Font(SectionTitleFont)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.96f, 1.0f)))
				]
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
				BuildModeSelector()
			]
		];
}
