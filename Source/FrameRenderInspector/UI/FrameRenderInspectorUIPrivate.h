#pragma once

#include "SFrameRenderInspectorUI.h"

#include "SFrameRenderInspectorPixelPicker.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/DefaultValueHelper.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
	const FName BufferMonoTextStyle(TEXT("NormalText"));
	const TCHAR* const LumenKeywords[] =
	{
		TEXT("Lumen"),
		TEXT("SurfaceCache"),
		TEXT("RadianceCache"),
		TEXT("ScreenProbe"),
		TEXT("CardPage"),
		TEXT("CardAtlas"),
		TEXT("Irradiance")
	};
	const TCHAR* const NaniteKeywords[] =
	{
		TEXT("Nanite"),
		TEXT("VisBuffer"),
		TEXT("ClusterPage"),
		TEXT("Hierarchy"),
		TEXT("CoarseMeshStreaming")
	};
	const TCHAR* const PostProcessKeywords[] =
	{
		TEXT("Tonemap"),
		TEXT("Bloom"),
		TEXT("PostProcess"),
		TEXT("PostOpaque"),
		TEXT("SceneColor"),
		TEXT("Velocity"),
		TEXT("DOF"),
		TEXT("TemporalAA"),
		TEXT("TSR"),
		TEXT("FXAA"),
		TEXT("MotionBlur"),
		TEXT("EyeAdaptation"),
		TEXT("Exposure")
	};
	const TCHAR* const ShadowKeywords[] =
	{
		TEXT("Shadow"),
		TEXT("ShadowDepth"),
		TEXT("ShadowMask"),
		TEXT("DistanceFieldShadow"),
		TEXT("CSM"),
		TEXT("Cascade")
	};
	const TCHAR* const VirtualShadowMapKeywords[] =
	{
		TEXT("VirtualShadowMap"),
		TEXT("VSM"),
		TEXT("ShadowPage"),
		TEXT("PhysicalPage"),
		TEXT("PageTable"),
		TEXT("Clipmap")
	};

	bool ContainsKeyword(const FString& Value, const TCHAR* const* Keywords, int32 KeywordCount)
	{
		for (int32 Index = 0; Index < KeywordCount; ++Index)
		{
			if (Value.Contains(Keywords[Index], ESearchCase::IgnoreCase))
			{
				return true;
			}
		}

		return false;
	}

	FString FormatBinary(uint32 Value)
	{
		FString Result;
		Result.Reserve(35);
		for (int32 BitIndex = 31; BitIndex >= 0; --BitIndex)
		{
			Result.AppendChar(((Value >> BitIndex) & 1u) != 0u ? TEXT('1') : TEXT('0'));
			if (BitIndex > 0 && BitIndex % 8 == 0)
			{
				Result.AppendChar(TEXT(' '));
			}
		}
		return Result;
	}

	template <typename NumericType>
	bool TryParseIntegerLiteral(const FString& InText, NumericType& OutValue)
	{
		const FString Trimmed = InText.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}

		if (Trimmed.StartsWith(TEXT("0x")) || Trimmed.StartsWith(TEXT("0X")))
		{
			const uint64 ParsedValue = FParse::HexNumber64(*Trimmed);
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}

		if constexpr (TIsSigned<NumericType>::Value)
		{
			int64 ParsedValue = 0;
			if (!LexTryParseString(ParsedValue, *Trimmed))
			{
				return false;
			}
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}
		else
		{
			uint64 ParsedValue = 0;
			if (!LexTryParseString(ParsedValue, *Trimmed))
			{
				return false;
			}
			OutValue = static_cast<NumericType>(ParsedValue);
			return true;
		}
	}
}
