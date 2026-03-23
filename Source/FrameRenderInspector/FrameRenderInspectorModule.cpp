#include "FrameRenderInspectorModule.h"
#include "SFrameRenderInspectorUI.h"
#include "FrameRenderInspectorCollector.h"
#include "FrameRenderInspectorPixelPickerModule.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

static const FName FrameRenderInspectorTabName("FrameRenderInspector");
static const TCHAR* FrameRenderInspectorSettingsSection = TEXT("/Script/FrameRenderInspector.FrameRenderInspectorSettings");

#define LOCTEXT_NAMESPACE "FFrameRenderInspectorModule"

#include "Module/FrameRenderInspectorModuleSettings.inl"
#include "Module/FrameRenderInspectorModuleUISync.inl"
#include "Module/FrameRenderInspectorModuleActions.inl"
#include "Module/FrameRenderInspectorModuleLifecycle.inl"

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFrameRenderInspectorModule, FrameRenderInspector)
