FSlateColor SFrameRenderInspectorUI::GetModeButtonColor(EInspectorMode Mode) const
{
	if (Mode == ActiveMode)
	{
		return FSlateColor(FLinearColor(0.23f, 0.48f, 0.86f));
	}

	return FSlateColor(FLinearColor(0.16f, 0.18f, 0.22f));
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

FText SFrameRenderInspectorUI::GetRenderOptionsPageText() const
{
	const int32 PageCount = GetRenderOptionsPageCount();
	const int32 CurrentPage = PageCount > 0 ? CurrentRenderOptionsPage + 1 : 0;
	return FText::FromString(FString::Printf(TEXT("Page %d/%d"), CurrentPage, PageCount));
}

int32 SFrameRenderInspectorUI::GetRenderOptionsPageCount() const
{
	return RenderOptionsPerPage > 0 ? FMath::Max(1, FMath::DivideAndRoundUp(FilteredRenderOptions.Num(), RenderOptionsPerPage)) : 1;
}

void SFrameRenderInspectorUI::ClampRenderOptionsPage()
{
	CurrentRenderOptionsPage = FMath::Clamp(CurrentRenderOptionsPage, 0, GetRenderOptionsPageCount() - 1);
}
