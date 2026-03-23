#pragma once

#include "CoreMinimal.h"
#include "FrameRenderInspectorPixelPickerTypes.h"
#include "Widgets/SCompoundWidget.h"

class FRAMERENDERINSPECTORPIXELPICKER_API SFrameRenderInspectorPixelPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFrameRenderInspectorPixelPicker) {}
	SLATE_END_ARGS()

	DECLARE_DELEGATE_TwoParams(FOnRequestPixelSample, int32, int32);
	DECLARE_DELEGATE(FOnBeginViewportPick);

	void Construct(const FArguments& InArgs);

	void SetOnRequestPixelSample(FOnRequestPixelSample InDelegate);
	void SetOnBeginViewportPick(FOnBeginViewportPick InDelegate);
	void SetPreviewSize(const FIntPoint& InPreviewSize);
	void SetSelectedTextureName(const FString& InTextureName);
	void SetSampleResult(const FTexturePixelSampleResult& InResult);
	void SetViewportPickArmed(bool bInViewportPickArmed);

private:
	FOnRequestPixelSample OnRequestPixelSampleDelegate;
	FOnBeginViewportPick OnBeginViewportPickDelegate;
	FString SelectedTextureName;
	FIntPoint PreviewSize = FIntPoint::ZeroValue;
	int32 SamplePixelX = 0;
	int32 SamplePixelY = 0;
	FTexturePixelSampleResult SampleResult;
	bool bViewportPickArmed = false;

	FReply OnSampleButtonClicked();
	FReply OnViewportPickButtonClicked();
	void OnSamplePixelXCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnSamplePixelYCommitted(const FText& NewText, ETextCommit::Type CommitType);
	FText GetPreviewSizeText() const;
	FText GetSamplePixelXText() const;
	FText GetSamplePixelYText() const;
	FText GetSampleResultText() const;
	FText GetSampleColorText() const;
	FLinearColor GetSampleColorBlock() const;
	FText GetViewportPickButtonText() const;
};
