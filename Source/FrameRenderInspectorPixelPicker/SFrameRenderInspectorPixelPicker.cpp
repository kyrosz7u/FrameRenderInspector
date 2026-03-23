#include "SFrameRenderInspectorPixelPicker.h"

#include "Misc/DefaultValueHelper.h"
#include "Styling/AppStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

void SFrameRenderInspectorPixelPicker::Construct(const FArguments& InArgs)
{
	const FSlateFontInfo ControlFont = FAppStyle::GetFontStyle("PropertyWindow.BoldFont");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(this, &SFrameRenderInspectorPixelPicker::GetPreviewSizeText)
				.Font(ControlFont)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Sample Pixel")))
				.OnClicked(this, &SFrameRenderInspectorPixelPicker::OnSampleButtonClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(this, &SFrameRenderInspectorPixelPicker::GetViewportPickButtonText)
				.OnClicked(this, &SFrameRenderInspectorPixelPicker::OnViewportPickButtonClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("X"))).Font(ControlFont)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SFrameRenderInspectorPixelPicker::GetSamplePixelXText)
				.OnTextCommitted(this, &SFrameRenderInspectorPixelPicker::OnSamplePixelXCommitted)
				.Font(ControlFont)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Y"))).Font(ControlFont)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SFrameRenderInspectorPixelPicker::GetSamplePixelYText)
				.OnTextCommitted(this, &SFrameRenderInspectorPixelPicker::OnSamplePixelYCommitted)
				.Font(ControlFont)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SColorBlock)
				.Color(this, &SFrameRenderInspectorPixelPicker::GetSampleColorBlock)
				.Size(FVector2D(28.0f, 18.0f))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SFrameRenderInspectorPixelPicker::GetSampleColorText)
				.Font(ControlFont)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(this, &SFrameRenderInspectorPixelPicker::GetSampleResultText)
			.Font(ControlFont)
		]
	];
}

void SFrameRenderInspectorPixelPicker::SetOnRequestPixelSample(FOnRequestPixelSample InDelegate)
{
	OnRequestPixelSampleDelegate = InDelegate;
}

void SFrameRenderInspectorPixelPicker::SetOnBeginViewportPick(FOnBeginViewportPick InDelegate)
{
	OnBeginViewportPickDelegate = InDelegate;
}

void SFrameRenderInspectorPixelPicker::SetPreviewSize(const FIntPoint& InPreviewSize)
{
	PreviewSize = InPreviewSize;
}

void SFrameRenderInspectorPixelPicker::SetSelectedTextureName(const FString& InTextureName)
{
	SelectedTextureName = InTextureName;
}

void SFrameRenderInspectorPixelPicker::SetSampleResult(const FTexturePixelSampleResult& InResult)
{
	SampleResult = InResult;
	SamplePixelX = InResult.SamplePixel.X;
	SamplePixelY = InResult.SamplePixel.Y;
	if (InResult.PreviewSize.X > 0 && InResult.PreviewSize.Y > 0)
	{
		PreviewSize = InResult.PreviewSize;
	}
}

void SFrameRenderInspectorPixelPicker::SetViewportPickArmed(bool bInViewportPickArmed)
{
	bViewportPickArmed = bInViewportPickArmed;
}

FReply SFrameRenderInspectorPixelPicker::OnSampleButtonClicked()
{
	if (OnRequestPixelSampleDelegate.IsBound())
	{
		OnRequestPixelSampleDelegate.Execute(SamplePixelX, SamplePixelY);
	}

	return FReply::Handled();
}

FReply SFrameRenderInspectorPixelPicker::OnViewportPickButtonClicked()
{
	if (OnBeginViewportPickDelegate.IsBound())
	{
		OnBeginViewportPickDelegate.Execute();
	}

	return FReply::Handled();
}

void SFrameRenderInspectorPixelPicker::OnSamplePixelXCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	int32 ParsedValue = 0;
	if (FDefaultValueHelper::ParseInt(NewText.ToString(), ParsedValue))
	{
		SamplePixelX = FMath::Max(0, ParsedValue);
	}
}

void SFrameRenderInspectorPixelPicker::OnSamplePixelYCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	int32 ParsedValue = 0;
	if (FDefaultValueHelper::ParseInt(NewText.ToString(), ParsedValue))
	{
		SamplePixelY = FMath::Max(0, ParsedValue);
	}
}

FText SFrameRenderInspectorPixelPicker::GetPreviewSizeText() const
{
	if (PreviewSize.X <= 0 || PreviewSize.Y <= 0)
	{
		return FText::FromString(TEXT("Preview not ready"));
	}

	return FText::FromString(FString::Printf(TEXT("%d x %d"), PreviewSize.X, PreviewSize.Y));
}

FText SFrameRenderInspectorPixelPicker::GetSamplePixelXText() const
{
	return FText::AsNumber(SamplePixelX);
}

FText SFrameRenderInspectorPixelPicker::GetSamplePixelYText() const
{
	return FText::AsNumber(SamplePixelY);
}

FText SFrameRenderInspectorPixelPicker::GetSampleResultText() const
{
	return FText::FromString(SampleResult.StatusMessage.IsEmpty() ? TEXT("No sample yet.") : SampleResult.StatusMessage);
}

FText SFrameRenderInspectorPixelPicker::GetSampleColorText() const
{
	return FText::FromString(FString::Printf(
		TEXT("RGBA %.4f, %.4f, %.4f, %.4f | #%02X%02X%02X%02X"),
		SampleResult.SampledColor.R,
		SampleResult.SampledColor.G,
		SampleResult.SampledColor.B,
		SampleResult.SampledColor.A,
		SampleResult.SampledColorLDR.R,
		SampleResult.SampledColorLDR.G,
		SampleResult.SampledColorLDR.B,
		SampleResult.SampledColorLDR.A));
}

FLinearColor SFrameRenderInspectorPixelPicker::GetSampleColorBlock() const
{
	return SampleResult.SampledColor;
}

FText SFrameRenderInspectorPixelPicker::GetViewportPickButtonText() const
{
	return bViewportPickArmed ? FText::FromString(TEXT("Click Viewport...")) : FText::FromString(TEXT("Pick From Viewport"));
}
