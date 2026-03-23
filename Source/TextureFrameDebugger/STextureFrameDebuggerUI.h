#pragma once

#include "CoreMinimal.h"
#include "TextureFrameDebuggerTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"

class TEXTUREFRAMEDEBUGGER_API STextureFrameDebuggerUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureFrameDebuggerUI) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Interface to update the texture dropdown
	void UpdateTextureOptions(const TArray<FString>& TextureNames, const FString& SelectedTextureName);
	void UpdateBufferOptions(const TArray<FBufferDebuggerItem>& BufferItems, const FString& SelectedBufferName);
	void UpdateRenderOptions(const TArray<FRenderOptionItem>& RenderOptions);
	void SetBufferReadbackResult(const FBufferReadbackResult& InReadbackResult);
	void SetOverlaySettings(float InOpacity, float InCoverage);
	void SetRangeState(float InMin, float InMax, bool bInHasRange, bool bInRangeLocked);

	// Delegate for when a texture is selected
	DECLARE_DELEGATE_OneParam(FOnTextureSelected, const FString& /*TextureName*/);
	DECLARE_DELEGATE_OneParam(FOnBufferSelected, const FString& /*BufferName*/);
	DECLARE_DELEGATE(FOnRefreshBuffer);
	DECLARE_DELEGATE_TwoParams(FOnRenderOptionBoolChanged, const FString& /*OptionName*/, bool /*bValue*/);
	DECLARE_DELEGATE_TwoParams(FOnRenderOptionValueCommitted, const FString& /*OptionName*/, const FString& /*ValueText*/);
	DECLARE_DELEGATE_OneParam(FOnOverlayOpacityChanged, float /*Opacity*/);
	DECLARE_DELEGATE_OneParam(FOnOverlayCoverageChanged, float /*Coverage*/);
	DECLARE_DELEGATE(FOnComputeVisibleRange);
	DECLARE_DELEGATE_OneParam(FOnRangeLockChanged, bool /*bLocked*/);
	DECLARE_DELEGATE_TwoParams(FOnRangeEdited, float /*Min*/, float /*Max*/);
	void SetOnTextureSelected(FOnTextureSelected InOnTextureSelected);
	void SetOnBufferSelected(FOnBufferSelected InOnBufferSelected);
	void SetOnRefreshBuffer(FOnRefreshBuffer InOnRefreshBuffer);
	void SetOnRenderOptionBoolChanged(FOnRenderOptionBoolChanged InOnRenderOptionBoolChanged);
	void SetOnRenderOptionValueCommitted(FOnRenderOptionValueCommitted InOnRenderOptionValueCommitted);
	void SetOnOverlayOpacityChanged(FOnOverlayOpacityChanged InOnOverlayOpacityChanged);
	void SetOnOverlayCoverageChanged(FOnOverlayCoverageChanged InOnOverlayCoverageChanged);
	void SetOnComputeVisibleRange(FOnComputeVisibleRange InOnComputeVisibleRange);
	void SetOnRangeLockChanged(FOnRangeLockChanged InOnRangeLockChanged);
	void SetOnRangeEdited(FOnRangeEdited InOnRangeEdited);

private:
	enum class EBufferDisplayFormat : uint8
	{
		Float,
		Int,
		UInt,
		Hex
	};

	struct FBufferRowEntry
	{
		uint32 Address = 0;
		TArray<int32> ElementIndices;
	};

	TArray<TSharedPtr<FString>> AllTextureOptions;
	TArray<TSharedPtr<FString>> FilteredTextureOptions;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> TextureComboBox;
	TSharedPtr<FString> SelectedTextureOption;
	FString SearchText;

	TArray<TSharedPtr<FBufferDebuggerItem>> AllBufferOptions;
	TArray<TSharedPtr<FBufferDebuggerItem>> FilteredBufferOptions;
	TSharedPtr<SComboBox<TSharedPtr<FBufferDebuggerItem>>> BufferComboBox;
	TSharedPtr<FBufferDebuggerItem> SelectedBufferOption;
	TArray<TSharedPtr<FString>> BufferFormatOptions;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> BufferFormatComboBox;
	TSharedPtr<FString> SelectedBufferFormatOption;
	FString BufferFilterText;
	FString JumpAddressText;
	FString SearchValueText;
	uint32 BufferStride = 0;
	uint32 BufferCount = 0;
	TArray<uint8> BufferData;
	FString BufferStatusMessage;
	int32 BufferRows = 8;
	int32 BufferColumns = 16;
	int32 CurrentBufferPage = 0;
	EBufferDisplayFormat BufferDisplayFormat = EBufferDisplayFormat::UInt;
	TArray<int32> SearchMatchIndices;
	int32 CurrentSearchMatchCursor = INDEX_NONE;
	TArray<TSharedPtr<FBufferRowEntry>> VisibleBufferRows;
	TSharedPtr<class SListView<TSharedPtr<FBufferRowEntry>>> BufferRowsView;
	bool bIsSyncingBufferSelection = false;

	TArray<TSharedPtr<FRenderOptionItem>> AllRenderOptions;
	TArray<TSharedPtr<FRenderOptionItem>> FilteredRenderOptions;
	TArray<TSharedPtr<FRenderOptionItem>> VisibleRenderOptions;
	TSharedPtr<class SListView<TSharedPtr<FRenderOptionItem>>> RenderOptionsView;
	FString RenderOptionFilterText;
	int32 RenderOptionsPerPage = 24;
	int32 CurrentRenderOptionsPage = 0;

	float OverlayOpacity = 1.0f;
	float OverlayCoverage = 0.5f;
	float RangeMin = 0.0f;
	float RangeMax = 1.0f;
	bool bHasRange = false;
	bool bRangeLocked = false;
	FOnTextureSelected OnTextureSelectedDelegate;
	FOnBufferSelected OnBufferSelectedDelegate;
	FOnRefreshBuffer OnRefreshBufferDelegate;
	FOnRenderOptionBoolChanged OnRenderOptionBoolChangedDelegate;
	FOnRenderOptionValueCommitted OnRenderOptionValueCommittedDelegate;
	FOnOverlayOpacityChanged OnOverlayOpacityChangedDelegate;
	FOnOverlayCoverageChanged OnOverlayCoverageChangedDelegate;
	FOnComputeVisibleRange OnComputeVisibleRangeDelegate;
	FOnRangeLockChanged OnRangeLockChangedDelegate;
	FOnRangeEdited OnRangeEditedDelegate;

	void RebuildFilteredOptions();
	void RebuildFilteredBufferOptions();
	void RebuildBufferRows();
	void RebuildFilteredRenderOptions();
	void RebuildVisibleRenderOptions();
	TSharedRef<SWidget> GenerateTextureOptionWidget(TSharedPtr<FString> Item) const;
	TSharedRef<SWidget> GenerateBufferOptionWidget(TSharedPtr<FBufferDebuggerItem> Item) const;
	TSharedRef<SWidget> GenerateBufferFormatOptionWidget(TSharedPtr<FString> Item) const;
	TSharedRef<class ITableRow> OnGenerateBufferRowWidget(TSharedPtr<FBufferRowEntry> Item, const TSharedRef<class STableViewBase>& OwnerTable);
	TSharedRef<class ITableRow> OnGenerateRenderOptionRowWidget(TSharedPtr<FRenderOptionItem> Item, const TSharedRef<class STableViewBase>& OwnerTable);
	void OnTextureSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void OnTextureFilterChanged(const FText& InFilterText);
	void OnBufferSelectionChanged(TSharedPtr<FBufferDebuggerItem> Item, ESelectInfo::Type SelectInfo);
	void OnBufferFilterChanged(const FText& InFilterText);
	void OnRenderOptionFilterChanged(const FText& InFilterText);
	void OnBufferFormatSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void OnOverlayOpacitySliderChanged(float NewValue);
	void OnOverlayCoverageSliderChanged(float NewValue);
	FReply OnComputeVisibleRangeClicked();
	FReply OnRefreshBufferClicked();
	FReply OnCopyBufferPageClicked();
	FReply OnPreviousBufferPageClicked();
	FReply OnNextBufferPageClicked();
	FReply OnGoToBufferAddressClicked();
	FReply OnSearchBufferClicked();
	FReply OnNextSearchMatchClicked();
	FReply OnPreviousRenderOptionsPageClicked();
	FReply OnNextRenderOptionsPageClicked();
	void OnRowsTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnColumnsTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnJumpAddressTextChanged(const FText& NewText);
	void OnSearchValueTextChanged(const FText& NewText);
	void OnRangeLockCheckStateChanged(ECheckBoxState NewState);
	void OnRangeMinTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnRangeMaxTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void BroadcastRangeEdited();
	FText GetSelectedTextureText() const;
	FText GetSelectedBufferText() const;
	FText GetBufferStrideText() const;
	FText GetBufferCountText() const;
	FText GetBufferPageText() const;
	FText GetBufferStatusText() const;
	FText GetRowsText() const;
	FText GetColumnsText() const;
	FText GetRenderOptionsPageText() const;
	FText GetOverlayOpacityText() const;
	FText GetOverlayCoverageText() const;
	FText GetRangeMinText() const;
	FText GetRangeMaxText() const;
	ECheckBoxState GetRangeLockCheckState() const;
	int32 GetBufferWordCount() const;
	int32 GetBufferPageCount() const;
	int32 GetBufferCellsPerPage() const;
	void ClampBufferPage();
	FString FormatBufferValue(int32 ElementIndex) const;
	FString BuildBufferValueTooltip(int32 ElementIndex) const;
	FText GetBufferValueText(int32 ElementIndex) const;
	FLinearColor GetBufferValueColor(int32 ElementIndex) const;
	bool TryGetBufferWord(int32 ElementIndex, uint32& OutValue) const;
	bool TryParseAddress(const FString& InText, uint32& OutAddress) const;
	bool TryParseSearchWord(const FString& InText, uint32& OutValue) const;
	void RebuildSearchMatches();
	void JumpToElementIndex(int32 ElementIndex);
	int32 GetRenderOptionsPageCount() const;
	void ClampRenderOptionsPage();
};
