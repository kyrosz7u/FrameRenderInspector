void FFrameRenderInspectorCollector::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	FAfterPassCallbackDelegateArray& InOutCallbacks,
	bool bIsPassEnabled)
{
	if (Pass != EPostProcessingPass::Tonemap)
	{
		return;
	}

	InOutCallbacks.Add(
		FAfterPassCallbackDelegate::CreateLambda(
			[this](FRDGBuilder& GraphBuilder,
				   const FSceneView& View,
				   const FPostProcessMaterialInputs& Inputs)
			{
				FScreenPassTexture SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
				const FString CurrentSelectedTexture = GetSelectedTextureName();

				if (!bIsCaptureEnabled.Load())
				{
					return SceneColor;
				}

				PollTexturePixelReadback();
				PollBufferReadback();

				TMap<FString, FIntPoint> CollectedInfo;
				TArray<FBufferDebuggerItem> BufferItems;
				FRDGTextureRef SelectedTexture = nullptr;
				FRDGBufferRef SelectedBuffer = nullptr;
				FString PendingBufferCaptureName;
				const bool bShouldCaptureBuffer = ConsumeBufferCaptureRequest(PendingBufferCaptureName);

				auto AddTextureIfValid = [&](const FString& Name, FRDGTextureRef Tex)
				{
					if (Tex)
					{
						CollectedInfo.Add(Name, Tex->Desc.Extent);

						if (!CurrentSelectedTexture.IsEmpty() && Name == CurrentSelectedTexture)
						{
							SelectedTexture = Tex;
						}
					}
				};

				for (const auto& Pair : GraphBuilder.GetPooledTextureOwnershipMap())
				{
					FRDGPooledTexture* PooledTex = Pair.Key;
					FRDGTexture* RDGTex = Pair.Value;

					if (RDGTex && PooledTex)
					{
						FString Name = RDGTex->Name;
						if (Name.IsEmpty())
						{
							Name = TEXT("Unnamed_Pooled");
						}
						if (!CollectedInfo.Contains(Name))
						{
							AddTextureIfValid(Name, RDGTex);
						}
					}
				}

				for (const auto& Pair : GraphBuilder.GetExternalTextures())
				{
					FRDGTexture* RDGTex = Pair.Value;
					if (RDGTex)
					{
						FString Name = RDGTex->Name;
						if (Name.IsEmpty())
						{
							Name = TEXT("External");
						}
						if (!CollectedInfo.Contains(Name))
						{
							AddTextureIfValid(Name, RDGTex);
						}
					}
				}

				TSet<FString> SeenBufferNames;
				auto AddBufferIfValid = [&](FRDGBufferRef Buffer)
				{
					if (!Buffer)
					{
						return;
					}

					FString BufferName = Buffer->Name;
					if (BufferName.IsEmpty())
					{
						BufferName = TEXT("UnnamedBuffer");
					}

					if (SeenBufferNames.Contains(BufferName))
					{
						return;
					}

					SeenBufferNames.Add(BufferName);

					FBufferDebuggerItem Item;
					Item.Name = BufferName;
					Item.Stride = Buffer->Desc.BytesPerElement;
					Item.Count = Buffer->Desc.NumElements;
					Item.NumBytes = Buffer->Desc.GetSize();
					BufferItems.Add(Item);

					if (!PendingBufferCaptureName.IsEmpty() && BufferName == PendingBufferCaptureName)
					{
						SelectedBuffer = Buffer;
					}
				};

				EnumerateRDGBuffers(GraphBuilder, [&AddBufferIfValid](FRDGBuffer* Buffer)
				{
					AddBufferIfValid(Buffer);
				});

				if (SelectedTexture && SelectedTexture != SceneColor.Texture)
				{
					const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
					DrawSelectedTexturePreview(GraphBuilder, ViewInfo, SelectedTexture, SceneColor);
				}

				if (bShouldCaptureBuffer && SelectedBuffer && BufferReadback.IsValid())
				{
					const uint32 NumBytes = SelectedBuffer->Desc.GetSize();
					AddReadbackBufferPass(GraphBuilder, RDG_EVENT_NAME("FrameRenderInspector.BufferReadback"), SelectedBuffer,
						[this, SelectedBuffer, NumBytes](FRHICommandList& RHICmdList)
						{
							BufferReadback->EnqueueCopy(RHICmdList, SelectedBuffer->GetRHI(), NumBytes);
							FScopeLock Lock(&SelectedTextureMutex);
							bBufferReadbackPending = true;
							BufferReadbackName = SelectedBuffer->Name;
							BufferReadbackStride = SelectedBuffer->Desc.BytesPerElement;
							BufferReadbackCount = SelectedBuffer->Desc.NumElements;
							BufferReadbackNumBytes = NumBytes;
						});
				}

				if (CollectedInfo.Num() > 0 || BufferItems.Num() > 0)
				{
					AsyncTask(ENamedThreads::GameThread, [this, CollectedInfo, BufferItems]()
					{
						ProcessCollectedResources(CollectedInfo, BufferItems);
					});
				}

				return SceneColor;
			}));
}

void FFrameRenderInspectorCollector::ProcessCollectedResources(const TMap<FString, FIntPoint>& TextureInfo, const TArray<FBufferDebuggerItem>& BufferItems)
{
	TArray<FString> CurrentTextureNames;
	CurrentTextureNames.Reserve(TextureInfo.Num());
	for (const auto& Pair : TextureInfo)
	{
		CurrentTextureNames.Add(Pair.Key);
	}
	CurrentTextureNames.Sort();

	{
		FScopeLock Lock(&SelectedTextureMutex);
		if (CurrentTextureNames == LastCollectedTextureNames && BufferItems == LastCollectedBuffers)
		{
			return;
		}

		LastCollectedTextureNames = CurrentTextureNames;
		LastCollectedBuffers = BufferItems;
	}

	TArray<FTextureDebuggerItem> Items;
	for (const auto& Pair : TextureInfo)
	{
		FTextureDebuggerItem Item;
		Item.Name = Pair.Key;
		Item.Size = Pair.Value;
		Item.Format = TEXT("Unknown");
		Items.Add(Item);
	}

	FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
	Module.UpdateUI(Items, BufferItems);
}

void FFrameRenderInspectorCollector::DrawSelectedTexturePreview(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTexture* SelectedTexture,
	const FScreenPassTexture& SceneColor)
{
	if (!SelectedTexture || !SceneColor.Texture)
	{
		return;
	}

	PollAutoRangeReadback();

	const FIntPoint SourceExtent = SelectedTexture->Desc.Extent;
	const FIntRect SceneViewRect = SceneColor.ViewRect;
	float CurrentOverlayOpacity = 1.0f;
	float CurrentOverlayCoverage = 0.5f;
	float CurrentAutoRangeMin = 0.0f;
	float CurrentAutoRangeMax = 1.0f;
	GetOverlaySettings(CurrentOverlayOpacity, CurrentOverlayCoverage, CurrentAutoRangeMin, CurrentAutoRangeMax);
	const bool bVisualizeAsSingleChannel = IsSingleChannelDebugFormat(SelectedTexture->Desc.Format);
	bool bApplyRangeNormalization = false;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		bApplyRangeNormalization = bHasAutoRange;
	}

	if (SourceExtent.X <= 0 || SourceExtent.Y <= 0 || SceneViewRect.Width() <= 0 || SceneViewRect.Height() <= 0 || CurrentOverlayOpacity <= 0.0f || CurrentOverlayCoverage <= 0.0f)
	{
		return;
	}

	const FIntPoint MaxPreviewSize(
		FMath::Clamp(FMath::FloorToInt(SceneViewRect.Width() * CurrentOverlayCoverage), 1, SceneViewRect.Width()),
		FMath::Clamp(FMath::FloorToInt(SceneViewRect.Height() * CurrentOverlayCoverage), 1, SceneViewRect.Height()));

	FIntRect InputRect(
		SceneViewRect.Min.X,
		SceneViewRect.Max.Y - MaxPreviewSize.Y,
		SceneViewRect.Min.X + MaxPreviewSize.X,
		SceneViewRect.Max.Y);

	InputRect.Clip(FIntRect(FIntPoint::ZeroValue, SourceExtent));

	if (InputRect.Width() <= 0 || InputRect.Height() <= 0)
	{
		return;
	}

	const FIntRect OutputRect(
		SceneViewRect.Min.X,
		SceneViewRect.Max.Y - InputRect.Height(),
		SceneViewRect.Min.X + InputRect.Width(),
		SceneViewRect.Max.Y);

	const FIntPoint PreviewSize(InputRect.Width(), InputRect.Height());
	bool bShouldReportPreviewSize = false;
	{
		FScopeLock Lock(&SelectedTextureMutex);
		if (LastPreviewSize != PreviewSize)
		{
			LastPreviewSize = PreviewSize;
			bShouldReportPreviewSize = true;
		}
	}

	if (bShouldReportPreviewSize)
	{
		AsyncTask(ENamedThreads::GameThread, [PreviewSize]()
		{
			FFrameRenderInspectorModule& Module = FModuleManager::GetModuleChecked<FFrameRenderInspectorModule>("FrameRenderInspector");
			Module.UpdateTexturePreviewSize(PreviewSize);
		});
	}

	if (ConsumeVisibleRangeUpdateRequest())
	{
		TStaticArray<uint32, 2> InitialRangeData;
		InitialRangeData[0] = EncodeOrderedFloat(FLT_MAX);
		InitialRangeData[1] = EncodeOrderedFloat(-FLT_MAX);

		FRDGBufferRef VisibleRangeBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("TextureFrame.VisibleRange"),
			sizeof(uint32),
			InitialRangeData.Num(),
			InitialRangeData.GetData(),
			sizeof(uint32) * InitialRangeData.Num());
		AddComputeVisibleRangePass(GraphBuilder, View, SelectedTexture, InputRect, bVisualizeAsSingleChannel, VisibleRangeBuffer);

		if (AutoRangeReadback.IsValid())
		{
			AddReadbackBufferPass(GraphBuilder, RDG_EVENT_NAME("TextureFrameVisibleRangeReadback"), VisibleRangeBuffer,
				[this, VisibleRangeBuffer](FRHICommandList& RHICmdList)
				{
					AutoRangeReadback->EnqueueCopy(RHICmdList, VisibleRangeBuffer->GetRHI(), 0u);
					FScopeLock Lock(&SelectedTextureMutex);
					bAutoRangeReadbackPending = true;
					bPendingVisibleRangeUpdate = false;
				});
		}
		else
		{
			FScopeLock Lock(&SelectedTextureMutex);
			bPendingVisibleRangeUpdate = false;
		}
	}

	FIntPoint RequestedPixel = FIntPoint::ZeroValue;
	if (ConsumeTexturePixelSampleRequest(RequestedPixel) && TexturePixelReadback.IsValid())
	{
		const FIntPoint ClampedPreviewPixel(
			FMath::Clamp(RequestedPixel.X, 0, PreviewSize.X - 1),
			FMath::Clamp(RequestedPixel.Y, 0, PreviewSize.Y - 1));
		const FIntPoint SourcePixelCoord(
			InputRect.Min.X + ClampedPreviewPixel.X,
			InputRect.Min.Y + ClampedPreviewPixel.Y);

		TStaticArray<uint32, 4> InitialPixelData;
		InitialPixelData[0] = 0u;
		InitialPixelData[1] = 0u;
		InitialPixelData[2] = 0u;
		InitialPixelData[3] = 0u;

		FRDGBufferRef SampledPixelBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("TextureFrame.SampledPixel"),
			sizeof(uint32),
			InitialPixelData.Num(),
			InitialPixelData.GetData(),
			sizeof(uint32) * InitialPixelData.Num());

		AddSamplePixelPass(
			GraphBuilder,
			View,
			SelectedTexture,
			SourcePixelCoord,
			bApplyRangeNormalization,
			bVisualizeAsSingleChannel,
			CurrentAutoRangeMin,
			CurrentAutoRangeMax,
			SampledPixelBuffer);

		AddReadbackBufferPass(GraphBuilder, RDG_EVENT_NAME("TextureFrameSamplePixelReadback"), SampledPixelBuffer,
			[this, SampledPixelBuffer, CurrentSelectedTexture = GetSelectedTextureName(), PreviewSize, ClampedPreviewPixel](FRHICommandList& RHICmdList)
			{
				TexturePixelReadback->EnqueueCopy(RHICmdList, SampledPixelBuffer->GetRHI(), sizeof(uint32) * 4);
				FScopeLock Lock(&SelectedTextureMutex);
				bTexturePixelReadbackPending = true;
				TexturePixelReadbackName = CurrentSelectedTexture;
				TexturePixelReadbackPreviewSize = PreviewSize;
				TexturePixelReadbackPixel = ClampedPreviewPixel;
			});
	}

	AddFrameRenderInspectorOverlayPass(
		GraphBuilder,
		View,
		SelectedTexture,
		InputRect,
		SceneColor.Texture,
		OutputRect,
		CurrentOverlayOpacity,
		bApplyRangeNormalization,
		bVisualizeAsSingleChannel,
		CurrentAutoRangeMin,
		CurrentAutoRangeMax);
}
