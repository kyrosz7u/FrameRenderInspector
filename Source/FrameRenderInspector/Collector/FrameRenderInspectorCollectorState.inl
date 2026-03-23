FFrameRenderInspectorCollector::FFrameRenderInspectorCollector(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
	, bIsCaptureEnabled(false)
{
	AutoRangeReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FrameRenderInspector.VisibleRangeReadback"));
	TexturePixelReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FrameRenderInspector.TexturePixelReadback"));
	BufferReadback = MakeUnique<FRHIGPUBufferReadback>(TEXT("FrameRenderInspector.BufferReadback"));
}

void FFrameRenderInspectorCollector::SetCaptureEnabled(bool bEnabled)
{
	bIsCaptureEnabled = bEnabled;
}

void FFrameRenderInspectorCollector::SetSelectedTexture(const FString& TextureName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	SelectedTextureName = TextureName;
}

void FFrameRenderInspectorCollector::SetSelectedBuffer(const FString& BufferName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	SelectedBufferName = BufferName;
}

void FFrameRenderInspectorCollector::SetOverlayOpacity(float InOpacity)
{
	FScopeLock Lock(&SelectedTextureMutex);
	OverlayOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
}

void FFrameRenderInspectorCollector::SetOverlayCoverage(float InCoverage)
{
	FScopeLock Lock(&SelectedTextureMutex);
	OverlayCoverage = FMath::Clamp(InCoverage, 0.0f, 1.0f);
}

void FFrameRenderInspectorCollector::SetRangeLocked(bool bLocked)
{
	FScopeLock Lock(&SelectedTextureMutex);
	bRangeLocked = bLocked;
}

void FFrameRenderInspectorCollector::SetManualRange(float InMin, float InMax)
{
	SetAutoRange(InMin, InMax);
}

void FFrameRenderInspectorCollector::SetAutoRange(float InMin, float InMax)
{
	FScopeLock Lock(&SelectedTextureMutex);
	const float ClampedMax = FMath::Max(InMin + UE_SMALL_NUMBER, InMax);
	AutoRangeMin = InMin;
	AutoRangeMax = ClampedMax;
	bHasAutoRange = true;
	UE_LOG(LogTemp, Log, TEXT("FrameRenderInspector Visible Range [%s]: Min=%g Max=%g"), *SelectedTextureName, AutoRangeMin, AutoRangeMax);
}

void FFrameRenderInspectorCollector::RequestVisibleRangeUpdate()
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!bRangeLocked)
	{
		bPendingVisibleRangeUpdate = true;
	}
}

void FFrameRenderInspectorCollector::RequestTexturePixelSample(int32 PixelX, int32 PixelY)
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!SelectedTextureName.IsEmpty())
	{
		PendingTexturePixel = FIntPoint(FMath::Max(0, PixelX), FMath::Max(0, PixelY));
		bPendingTexturePixelSample = true;
	}
}

void FFrameRenderInspectorCollector::RequestBufferCapture()
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!SelectedBufferName.IsEmpty())
	{
		bPendingBufferCapture = true;
	}
}

bool FFrameRenderInspectorCollector::ConsumeVisibleRangeUpdateRequest()
{
	FScopeLock Lock(&SelectedTextureMutex);
	const bool bWasPending = bPendingVisibleRangeUpdate;
	bPendingVisibleRangeUpdate = false;
	return bWasPending;
}

bool FFrameRenderInspectorCollector::ConsumeTexturePixelSampleRequest(FIntPoint& OutPixelCoord)
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!bPendingTexturePixelSample || SelectedTextureName.IsEmpty())
	{
		return false;
	}

	OutPixelCoord = PendingTexturePixel;
	bPendingTexturePixelSample = false;
	return true;
}

bool FFrameRenderInspectorCollector::ConsumeBufferCaptureRequest(FString& OutBufferName)
{
	FScopeLock Lock(&SelectedTextureMutex);
	if (!bPendingBufferCapture || SelectedBufferName.IsEmpty())
	{
		return false;
	}

	bPendingBufferCapture = false;
	OutBufferName = SelectedBufferName;
	return true;
}

FString FFrameRenderInspectorCollector::GetSelectedTextureName() const
{
	FScopeLock Lock(&SelectedTextureMutex);
	return SelectedTextureName;
}

FString FFrameRenderInspectorCollector::GetSelectedBufferName() const
{
	FScopeLock Lock(&SelectedTextureMutex);
	return SelectedBufferName;
}

void FFrameRenderInspectorCollector::GetRangeState(float& OutMin, float& OutMax, bool& bOutHasRange, bool& bOutRangeLocked) const
{
	FScopeLock Lock(&SelectedTextureMutex);
	OutMin = AutoRangeMin;
	OutMax = AutoRangeMax;
	bOutHasRange = bHasAutoRange;
	bOutRangeLocked = bRangeLocked;
}

void FFrameRenderInspectorCollector::GetOverlaySettings(float& OutOpacity, float& OutCoverage, float& OutAutoRangeMin, float& OutAutoRangeMax) const
{
	FScopeLock Lock(&SelectedTextureMutex);
	OutOpacity = OverlayOpacity;
	OutCoverage = OverlayCoverage;
	OutAutoRangeMin = AutoRangeMin;
	OutAutoRangeMax = AutoRangeMax;
}

static bool IsSingleChannelDebugFormat(EPixelFormat Format)
{
	switch (Format)
	{
	case PF_G8:
	case PF_G16:
	case PF_DepthStencil:
	case PF_ShadowDepth:
	case PF_D24:
	case PF_R32_FLOAT:
	case PF_R16F:
	case PF_R16F_FILTER:
	case PF_R16_UINT:
	case PF_R16_SINT:
	case PF_R32_UINT:
	case PF_R32_SINT:
	case PF_R8:
	case PF_R8_UINT:
	case PF_R8_SINT:
	case PF_L8:
		return true;
	default:
		return false;
	}
}
