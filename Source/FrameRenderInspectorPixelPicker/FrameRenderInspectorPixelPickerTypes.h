#pragma once

#include "CoreMinimal.h"

struct FTexturePixelSampleResult
{
	FString Name;
	FIntPoint PreviewSize = FIntPoint::ZeroValue;
	FIntPoint SamplePixel = FIntPoint::ZeroValue;
	FLinearColor SampledColor = FLinearColor::Black;
	FColor SampledColorLDR = FColor::Black;
	FString StatusMessage;
	bool bSucceeded = false;
};
