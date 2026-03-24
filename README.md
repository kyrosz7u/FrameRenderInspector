# Frame Render Inspector

[中文](#中文说明) | [English](#english)

## 中文说明

### 简介

`Frame Render Inspector` 是一个 Unreal Engine 渲染调试插件，用来在编辑器中快速检查当前帧的纹理、RDG Buffer 和常用渲染选项。

它的目标不是替代 RenderDoc，而是提供一个更轻量、更直接的日常调试面板，方便在编辑器里快速查看：

- 当前帧可用纹理
- RDG / 渲染缓冲区内容
- 常用 `r.*` 渲染选项
- 选中纹理在 viewport 中的可视化结果
- 指定像素的颜色采样结果

### 功能

- 纹理检查
  - 按名称过滤纹理
  - 选择纹理并在 viewport 中显示 `DebuggerRT` 叠加预览
  - 调整 `Overlay Opacity`
  - 调整 `Overlay Coverage`
  - 支持 `Visualize In Viewport` 开关
  - 计算并锁定可见范围（Visible Range）

- Buffer 检查
  - 枚举当前帧 RDG Buffer
  - 查看 stride、count、总字节数
  - 以 `Float / Int / UInt / Hex` 格式查看内容
  - 支持分页、跳转地址、关键值查找、复制当前页

- Render Options 检查
  - 枚举常见 `r.*` 控制台变量
  - 直接查看和修改 `bool / int / float` 类型选项
  - 支持过滤和分页

- 像素取样
  - 对当前 `DebuggerRT` 预览中的像素进行采样
  - 支持手动输入像素坐标
  - 支持在编辑器 viewport 中点击取样
  - 显示线性颜色、LDR 颜色和采样状态

- 模式过滤
  - `All`
  - `General`
  - `Lumen`
  - `Nanite`
  - `PostProcess`
  - `Shadow`
  - `Virtual Shadow Map`

- 配置持久化
  - 自动保存 Inspector 状态
  - 默认恢复上一次使用的设置

### 安装

1. 将插件放到项目目录：

```text
<YourProject>/Plugins/FrameRenderInspector
```

2. 重新生成工程文件或直接打开项目。
3. 编译项目。
4. 启动编辑器。

### 使用方式

1. 在编辑器顶部菜单中打开：

```text
Tools -> Frame Render Inspector
```

2. 在 `Textures` 区选择要观察的纹理。
3. 根据需要启用或关闭 `Visualize In Viewport`。
4. 调整 overlay 参数，查看 viewport 中的调试结果。
5. 在 `Buffers` 区查看 RDG Buffer 内容。
6. 在 `Render Options` 区直接修改常用渲染变量。
7. 在 `Pixel Picker` 区对 `DebuggerRT` 进行像素采样。

### 截图

发布到 GitHub 前，建议在这里加入编辑器截图、viewport overlay 截图和 buffer 检查截图。

当前仓库里已有一些可选图片文件，可按需整理后再引用：

- `view.png`
- `ue.png`
- `example.png`

### 项目结构

当前代码按功能拆分，便于维护和扩展：

```text
Source/
  FrameRenderInspector/
    Collector/   -> 采集、读回、viewport overlay
    Module/      -> 生命周期、设置、UI 同步、交互动作
    UI/
      Common/    -> UI 装配
      Texture/   -> 纹理与 overlay 相关 UI
      Buffer/    -> Buffer 检查 UI
      Options/   -> Render Options 与模式过滤 UI

  FrameRenderInspectorPixelPicker/
    -> 像素取样模块
```

### 适用场景

- 快速确认某张调试纹理是否正确输出
- 查看单通道贴图、阴影贴图、Lumen / Nanite 中间结果
- 检查 RDG Buffer 的布局和内容
- 临时调整常见渲染变量，观察画面变化
- 在编辑器里直接做像素级取样验证

### 注意事项

- 这是一个编辑器内调试插件，适合开发阶段使用。
- `Render Options` 面板会直接修改运行中的控制台变量。
- 某些纹理或 Buffer 是否能被看到，取决于当前帧和具体渲染路径。
- viewport overlay 预览会受当前视口尺寸、有效内容区域和 overlay 参数影响。

### 已知问题

- 某些渲染资源只在特定 pass 或特定帧中存在，因此列表内容会随视图和场景状态变化。
- RDG Buffer 的可见性依赖当前渲染路径和资源生命周期。
- viewport overlay 的可视范围仍然受视口尺寸和有效内容区域约束。

### 后续计划

- 增加更多模式预设和更细粒度的过滤规则
- 继续优化 viewport overlay 的可视化行为
- 增加更多 RDG / 渲染资源分类能力
- 根据项目需求继续整理 UI 和模块边界

### 许可

当前仓库还没有附带正式的开源许可证文件。

如果你准备公开发布到 GitHub，建议在发布前补充一个明确的许可证，例如：

- `MIT`
- `BSD-3-Clause`
- `Apache-2.0`

---

## English

### Overview

`Frame Render Inspector` is a lightweight Unreal Engine rendering debug plugin for inspecting frame textures, RDG buffers, and common render options directly inside the editor.

It is not intended to replace RenderDoc. The goal is to provide a faster day-to-day inspection tool that lets you quickly:

- browse frame textures
- inspect RDG / render buffers
- tweak common `r.*` render options
- visualize the selected texture in the editor viewport
- sample exact pixels from the preview

### Features

- Texture inspection
  - filter textures by name
  - select a texture and display it as a `DebuggerRT` viewport overlay
  - adjust `Overlay Opacity`
  - adjust `Overlay Coverage`
  - toggle `Visualize In Viewport`
  - compute and lock visible range

- Buffer inspection
  - enumerate RDG buffers from the current frame
  - inspect stride, count, and total byte size
  - display values as `Float / Int / UInt / Hex`
  - paginate, jump to address, search for values, and copy the current page

- Render options inspection
  - enumerate common `r.*` console variables
  - inspect and edit `bool / int / float` values directly
  - filter and paginate entries

- Pixel sampling
  - sample pixels from the current `DebuggerRT` preview
  - sample by manual pixel coordinates
  - sample by clicking inside the editor viewport
  - view linear color, LDR color, and sampling status

- Mode-based filtering
  - `All`
  - `General`
  - `Lumen`
  - `Nanite`
  - `PostProcess`
  - `Shadow`
  - `Virtual Shadow Map`

- Persistent state
  - inspector state is saved automatically
  - the previous session is restored by default

### Installation

1. Place the plugin under:

```text
<YourProject>/Plugins/FrameRenderInspector
```

2. Regenerate project files if needed, or open the project directly.
3. Build the project.
4. Launch the editor.

### Usage

1. Open the tool from:

```text
Tools -> Frame Render Inspector
```

2. Select a texture in the `Textures` section.
3. Enable or disable `Visualize In Viewport` depending on whether you want the overlay drawn in the viewport.
4. Adjust overlay parameters and inspect the result in the editor viewport.
5. Inspect RDG buffers in the `Buffers` section.
6. Edit common render variables in the `Render Options` section.
7. Sample pixels from the `Pixel Picker` panel.

### Screenshots

Before publishing to GitHub, it is recommended to add editor screenshots, viewport overlay screenshots, and buffer inspection screenshots here.

The repository already contains a few local image candidates that can be organized and referenced later:

- `view.png`
- `ue.png`
- `example.png`

### Project Layout

The codebase is organized by feature area for readability and maintenance:

```text
Source/
  FrameRenderInspector/
    Collector/   -> capture, readback, viewport overlay
    Module/      -> lifecycle, settings, UI sync, actions
    UI/
      Common/    -> UI assembly
      Texture/   -> texture and overlay UI
      Buffer/    -> buffer inspection UI
      Options/   -> render options and mode filters

  FrameRenderInspectorPixelPicker/
    -> pixel sampling module
```

### Good Use Cases

- quickly verifying whether a debug texture is produced correctly
- inspecting single-channel textures, shadow maps, and intermediate Lumen / Nanite results
- checking RDG buffer layout and content
- temporarily changing render variables and observing the result
- validating exact pixel values directly inside the editor

### Notes

- This plugin is intended for editor-side debugging during development.
- The `Render Options` panel writes directly to live console variables.
- Texture and buffer visibility depends on the current frame and render path.
- Viewport overlay size depends on viewport size, valid content region, and overlay settings.

### Known Issues

- Some render resources only exist in specific passes or specific frames, so the visible list can change with scene and view state.
- RDG buffer visibility depends on the current render path and resource lifetime.
- Viewport overlay bounds are still constrained by viewport size and the valid content region.

### Roadmap

- add more mode presets and finer-grained filter rules
- continue improving viewport overlay behavior
- add richer RDG / render resource categorization
- keep refining UI and module boundaries as the tool grows

### License

This repository does not currently include a formal open-source license file.

If you plan to publish it on GitHub, it is recommended to add a clear license before release, for example:

- `MIT`
- `BSD-3-Clause`
- `Apache-2.0`
