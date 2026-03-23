#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRAMERENDERINSPECTORPIXELPICKER_API FFrameRenderInspectorPixelPickerModule : public IModuleInterface
{
public:
	DECLARE_DELEGATE_ThreeParams(FOnViewportPixelPicked, bool, int32, int32);

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void ArmViewportPicker(const FIntPoint& InPreviewSize, FOnViewportPixelPicked InOnPicked);
	void DisarmViewportPicker();
	bool IsViewportPickerArmed() const;

private:
	class FViewportPixelPickerInputProcessor;

	friend class FViewportPixelPickerInputProcessor;

	void HandleViewportClick(bool bSucceeded, int32 PixelX, int32 PixelY);

	TSharedPtr<FViewportPixelPickerInputProcessor> InputProcessor;
	FOnViewportPixelPicked OnViewportPixelPicked;
	FIntPoint ArmedPreviewSize = FIntPoint::ZeroValue;
	bool bViewportPickerArmed = false;
};
