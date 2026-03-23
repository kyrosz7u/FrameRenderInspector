#include "FrameRenderInspectorPixelPickerModule.h"

#include "Editor.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

class FFrameRenderInspectorPixelPickerModule::FViewportPixelPickerInputProcessor
	: public IInputProcessor
{
public:
	explicit FViewportPixelPickerInputProcessor(FFrameRenderInspectorPixelPickerModule* InOwner)
		: Owner(InOwner)
	{
	}

	void SetOwner(FFrameRenderInspectorPixelPickerModule* InOwner)
	{
		Owner = InOwner;
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (!Owner || !Owner->bViewportPickerArmed || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return false;
		}

		bool bSucceeded = false;
		int32 PixelX = 0;
		int32 PixelY = 0;

		if (GEditor && GEditor->GetActiveViewport())
		{
			FViewport* ActiveViewport = GEditor->GetActiveViewport();
			const FIntPoint ViewportSize = ActiveViewport->GetSizeXY();
			const int32 MouseX = ActiveViewport->GetMouseX();
			const int32 MouseY = ActiveViewport->GetMouseY();
			const int32 PreviewWidth = Owner->ArmedPreviewSize.X;
			const int32 PreviewHeight = Owner->ArmedPreviewSize.Y;
			const int32 PreviewTop = ViewportSize.Y - PreviewHeight;

			if (PreviewWidth > 0 && PreviewHeight > 0 &&
				MouseX >= 0 && MouseX < PreviewWidth &&
				MouseY >= PreviewTop && MouseY < ViewportSize.Y)
			{
				bSucceeded = true;
				PixelX = MouseX;
				PixelY = MouseY - PreviewTop;
			}
		}

		Owner->HandleViewportClick(bSucceeded, PixelX, PixelY);
		return false;
	}

	virtual const TCHAR* GetDebugName() const override
	{
		return TEXT("FrameRenderInspectorPixelPicker");
	}

private:
	FFrameRenderInspectorPixelPickerModule* Owner = nullptr;
};

void FFrameRenderInspectorPixelPickerModule::StartupModule()
{
	InputProcessor = MakeShared<FViewportPixelPickerInputProcessor>(this);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
	}
}

void FFrameRenderInspectorPixelPickerModule::ShutdownModule()
{
	if (InputProcessor.IsValid())
	{
		InputProcessor->SetOwner(nullptr);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
		}
		InputProcessor.Reset();
	}
}

void FFrameRenderInspectorPixelPickerModule::ArmViewportPicker(const FIntPoint& InPreviewSize, FOnViewportPixelPicked InOnPicked)
{
	ArmedPreviewSize = InPreviewSize;
	OnViewportPixelPicked = InOnPicked;
	bViewportPickerArmed = InPreviewSize.X > 0 && InPreviewSize.Y > 0;
}

void FFrameRenderInspectorPixelPickerModule::DisarmViewportPicker()
{
	bViewportPickerArmed = false;
	ArmedPreviewSize = FIntPoint::ZeroValue;
	OnViewportPixelPicked.Unbind();
}

bool FFrameRenderInspectorPixelPickerModule::IsViewportPickerArmed() const
{
	return bViewportPickerArmed;
}

void FFrameRenderInspectorPixelPickerModule::HandleViewportClick(bool bSucceeded, int32 PixelX, int32 PixelY)
{
	const FOnViewportPixelPicked CompletionDelegate = OnViewportPixelPicked;
	DisarmViewportPicker();

	if (CompletionDelegate.IsBound())
	{
		CompletionDelegate.Execute(bSucceeded, PixelX, PixelY);
	}
}

IMPLEMENT_MODULE(FFrameRenderInspectorPixelPickerModule, FrameRenderInspectorPixelPicker)
