#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"

// Structure to hold texture information for the UI
struct FTextureDebuggerItem
{
	FString Name;
	FIntPoint Size;
	FString Format;
};

class TEXTUREFRAMEDEBUGGER_API STextureFrameDebuggerUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureFrameDebuggerUI) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Interface to update the texture dropdown
	void UpdateTextureOptions(const TArray<FString>& TextureNames, const FString& SelectedTextureName);
	void SetOverlaySettings(float InOpacity, float InCoverage);
	void SetRangeState(float InMin, float InMax, bool bInHasRange, bool bInRangeLocked);

	// Delegate for when a texture is selected
	DECLARE_DELEGATE_OneParam(FOnTextureSelected, const FString& /*TextureName*/);
	DECLARE_DELEGATE_OneParam(FOnOverlayOpacityChanged, float /*Opacity*/);
	DECLARE_DELEGATE_OneParam(FOnOverlayCoverageChanged, float /*Coverage*/);
	DECLARE_DELEGATE(FOnComputeVisibleRange);
	DECLARE_DELEGATE_OneParam(FOnRangeLockChanged, bool /*bLocked*/);
	DECLARE_DELEGATE_TwoParams(FOnRangeEdited, float /*Min*/, float /*Max*/);
	void SetOnTextureSelected(FOnTextureSelected InOnTextureSelected);
	void SetOnOverlayOpacityChanged(FOnOverlayOpacityChanged InOnOverlayOpacityChanged);
	void SetOnOverlayCoverageChanged(FOnOverlayCoverageChanged InOnOverlayCoverageChanged);
	void SetOnComputeVisibleRange(FOnComputeVisibleRange InOnComputeVisibleRange);
	void SetOnRangeLockChanged(FOnRangeLockChanged InOnRangeLockChanged);
	void SetOnRangeEdited(FOnRangeEdited InOnRangeEdited);

private:
	TArray<TSharedPtr<FString>> AllTextureOptions;
	TArray<TSharedPtr<FString>> FilteredTextureOptions;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> TextureComboBox;
	TSharedPtr<FString> SelectedTextureOption;
	FString SearchText;
	float OverlayOpacity = 1.0f;
	float OverlayCoverage = 0.5f;
	float RangeMin = 0.0f;
	float RangeMax = 1.0f;
	bool bHasRange = false;
	bool bRangeLocked = false;
	FOnTextureSelected OnTextureSelectedDelegate;
	FOnOverlayOpacityChanged OnOverlayOpacityChangedDelegate;
	FOnOverlayCoverageChanged OnOverlayCoverageChangedDelegate;
	FOnComputeVisibleRange OnComputeVisibleRangeDelegate;
	FOnRangeLockChanged OnRangeLockChangedDelegate;
	FOnRangeEdited OnRangeEditedDelegate;

	void RebuildFilteredOptions();
	TSharedRef<SWidget> GenerateTextureOptionWidget(TSharedPtr<FString> Item) const;
	void OnTextureSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void OnTextureFilterChanged(const FText& InFilterText);
	void OnOverlayOpacitySliderChanged(float NewValue);
	void OnOverlayCoverageSliderChanged(float NewValue);
	FReply OnComputeVisibleRangeClicked();
	void OnRangeLockCheckStateChanged(ECheckBoxState NewState);
	void OnRangeMinTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnRangeMaxTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void BroadcastRangeEdited();
	FText GetSelectedTextureText() const;
	FText GetOverlayOpacityText() const;
	FText GetOverlayCoverageText() const;
	FText GetRangeMinText() const;
	FText GetRangeMaxText() const;
	ECheckBoxState GetRangeLockCheckState() const;
};
