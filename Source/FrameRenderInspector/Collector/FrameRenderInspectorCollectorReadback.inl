void FFrameRenderInspectorCollector::PollAutoRangeReadback()
{
	if (!AutoRangeReadback.IsValid())
	{
		return;
	}

	bool bIsReady = false;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bIsReady = bAutoRangeReadbackPending && AutoRangeReadback->IsReady();
	}

	if (!bIsReady)
	{
		return;
	}

	const uint32* RangeData = static_cast<const uint32*>(AutoRangeReadback->Lock(sizeof(uint32) * 2));
	if (RangeData)
	{
		const float MinValue = DecodeOrderedFloat(RangeData[0]);
		const float MaxValue = DecodeOrderedFloat(RangeData[1]);
		if (FMath::IsFinite(MinValue) && FMath::IsFinite(MaxValue) && MinValue <= MaxValue)
		{
			SetAutoRange(MinValue, MaxValue);
		}
	}
	AutoRangeReadback->Unlock();

	FScopeLock Lock(&SelectedTextureMutex);
	bAutoRangeReadbackPending = false;
}

void FFrameRenderInspectorCollector::PollTexturePixelReadback()
{
	if (!TexturePixelReadback.IsValid())
	{
		return;
	}

	bool bIsReady = false;
	FString CompletedTextureName;
	FIntPoint CompletedPreviewSize = FIntPoint::ZeroValue;
	FIntPoint CompletedPixel = FIntPoint::ZeroValue;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bIsReady = bTexturePixelReadbackPending && TexturePixelReadback->IsReady();
		CompletedTextureName = TexturePixelReadbackName;
		CompletedPreviewSize = TexturePixelReadbackPreviewSize;
		CompletedPixel = TexturePixelReadbackPixel;
	}

	if (!bIsReady)
	{
		return;
	}

	FTexturePixelSampleResult Result;
	Result.Name = CompletedTextureName;
	Result.PreviewSize = CompletedPreviewSize;
	Result.SamplePixel = CompletedPixel;

	const uint32* SampleData = static_cast<const uint32*>(TexturePixelReadback->Lock(sizeof(uint32) * 4));
	if (SampleData)
	{
		Result.SampledColor = FLinearColor(
			DecodeRawFloat(SampleData[0]),
			DecodeRawFloat(SampleData[1]),
			DecodeRawFloat(SampleData[2]),
			DecodeRawFloat(SampleData[3]));
		Result.SampledColorLDR = Result.SampledColor.ToFColor(true);
		Result.bSucceeded = true;
		Result.StatusMessage = FString::Printf(
			TEXT("Sampled %s at (%d, %d) in %d x %d DebuggerRT."),
			*CompletedTextureName,
			CompletedPixel.X,
			CompletedPixel.Y,
			CompletedPreviewSize.X,
			CompletedPreviewSize.Y);
	}
	else
	{
		Result.StatusMessage = FString::Printf(TEXT("Pixel sample failed for %s."), *CompletedTextureName);
	}

	TexturePixelReadback->Unlock();

	{
		FScopeLock Lock(&SelectedTextureMutex);
		bTexturePixelReadbackPending = false;
		TexturePixelReadbackName.Reset();
		TexturePixelReadbackPreviewSize = FIntPoint::ZeroValue;
		TexturePixelReadbackPixel = FIntPoint::ZeroValue;
	}

	AsyncTask(ENamedThreads::GameThread, [Result = MoveTemp(Result)]() mutable
	{
		FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
		Module.UpdateTexturePixelSample(Result);
	});
}

void FFrameRenderInspectorCollector::PollBufferReadback()
{
	if (!BufferReadback.IsValid())
	{
		return;
	}

	bool bIsReady = false;
	FString CompletedBufferName;
	uint32 CompletedStride = 0;
	uint32 CompletedCount = 0;
	uint32 CompletedNumBytes = 0;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bIsReady = bBufferReadbackPending && BufferReadback->IsReady();
		CompletedBufferName = BufferReadbackName;
		CompletedStride = BufferReadbackStride;
		CompletedCount = BufferReadbackCount;
		CompletedNumBytes = BufferReadbackNumBytes;
	}

	if (!bIsReady)
	{
		return;
	}

	FBufferReadbackResult Result;
	Result.Name = CompletedBufferName;
	Result.Stride = CompletedStride;
	Result.Count = CompletedCount;

	const void* LockedData = BufferReadback->Lock(CompletedNumBytes);
	if (LockedData && CompletedNumBytes > 0)
	{
		Result.Data.SetNumUninitialized(CompletedNumBytes);
		FMemory::Memcpy(Result.Data.GetData(), LockedData, CompletedNumBytes);
		Result.bSucceeded = true;
		Result.StatusMessage = FString::Printf(TEXT("Read back %u bytes from %s."), CompletedNumBytes, *CompletedBufferName);
	}
	else
	{
		Result.StatusMessage = FString::Printf(TEXT("Readback failed for %s."), *CompletedBufferName);
	}

	BufferReadback->Unlock();

	{
		FScopeLock Lock(&SelectedTextureMutex);
		bBufferReadbackPending = false;
		BufferReadbackName.Reset();
		BufferReadbackStride = 0;
		BufferReadbackCount = 0;
		BufferReadbackNumBytes = 0;
	}

	AsyncTask(ENamedThreads::GameThread, [Result = MoveTemp(Result)]() mutable
	{
		FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
		Module.UpdateBufferReadback(Result);
	});
}
