void FFrameRenderInspectorModule::UpdateUI(const TArray<FTextureDebuggerItem>& TextureItems, const TArray<FBufferDebuggerItem>& BufferItems)
{
	TArray<FString> TextureNames;
	TextureNames.Reserve(TextureItems.Num());

	for (const FTextureDebuggerItem& Item : TextureItems)
	{
		TextureNames.Add(Item.Name);
	}

	TextureNames.Sort();
	if (TextureNames != CachedTextureNames)
	{
		CachedTextureNames = MoveTemp(TextureNames);

		if (DebuggerUI.IsValid())
		{
			DebuggerUI->UpdateTextureOptions(CachedTextureNames, SelectedTextureName);
		}
	}

	CachedBufferItems = BufferItems;
	CachedBufferItems.Sort([](const FBufferDebuggerItem& A, const FBufferDebuggerItem& B)
	{
		return A.Name < B.Name;
	});

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->UpdateBufferOptions(CachedBufferItems, SelectedBufferName);
	}

	if (Collector.IsValid())
	{
		const float PreviousRangeMin = CurrentRangeMin;
		const float PreviousRangeMax = CurrentRangeMax;
		const bool bPreviousHasRange = bHasRange;
		const bool bPreviousRangeLocked = bRangeLocked;

		Collector->GetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
		const bool bRangeStateChanged =
			!bUIHasSyncedRangeState ||
			!FMath::IsNearlyEqual(CurrentRangeMin, PreviousRangeMin) ||
			!FMath::IsNearlyEqual(CurrentRangeMax, PreviousRangeMax) ||
			bHasRange != bPreviousHasRange ||
			bRangeLocked != bPreviousRangeLocked;

		if (DebuggerUI.IsValid() && bRangeStateChanged)
		{
			DebuggerUI->SetRangeState(CurrentRangeMin, CurrentRangeMax, bHasRange, bRangeLocked);
			bUIHasSyncedRangeState = true;
		}
	}
}

void FFrameRenderInspectorModule::UpdateBufferReadback(const FBufferReadbackResult& ReadbackResult)
{
	LatestBufferReadback = ReadbackResult;
	bHasBufferReadback = true;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetBufferReadbackResult(ReadbackResult);
	}
}

void FFrameRenderInspectorModule::UpdateTexturePreviewSize(const FIntPoint& PreviewSize)
{
	CurrentTexturePreviewSize = PreviewSize;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetTexturePreviewSize(PreviewSize);
	}
}

void FFrameRenderInspectorModule::UpdateTexturePixelSample(const FTexturePixelSampleResult& SampleResult)
{
	LatestTexturePixelSample = SampleResult;
	bHasTexturePixelSample = true;

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->SetTexturePixelSampleResult(SampleResult);
	}
}

void FFrameRenderInspectorModule::RefreshRenderOptions()
{
	CachedRenderOptions.Empty();

	IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
		FConsoleObjectVisitor::CreateLambda([this](const TCHAR* Name, IConsoleObject* Obj)
		{
			if (!Name || !Obj)
			{
				return;
			}

			IConsoleVariable* CVar = Obj->AsVariable();
			if (!CVar)
			{
				return;
			}

			FRenderOptionItem Item;
			Item.Name = Name;

			if (Obj->IsVariableBool())
			{
				Item.ValueType = ERenderOptionValueType::Bool;
				Item.bBoolValue = CVar->GetBool();
				Item.ValueText = Item.bBoolValue ? TEXT("1") : TEXT("0");
			}
			else if (Obj->IsVariableInt())
			{
				Item.ValueType = ERenderOptionValueType::Int;
				Item.ValueText = FString::Printf(TEXT("%d"), CVar->GetInt());
			}
			else
			{
				Item.ValueType = ERenderOptionValueType::Float;
				Item.ValueText = FString::Printf(TEXT("%g"), CVar->GetFloat());
			}

			CachedRenderOptions.Add(MoveTemp(Item));
		}),
		TEXT("r."));

	CachedRenderOptions.Sort([](const FRenderOptionItem& A, const FRenderOptionItem& B)
	{
		return A.Name < B.Name;
	});

	if (DebuggerUI.IsValid())
	{
		DebuggerUI->UpdateRenderOptions(CachedRenderOptions);
	}
}
