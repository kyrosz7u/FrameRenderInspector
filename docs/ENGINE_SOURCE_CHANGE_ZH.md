# Engine Source Change

[English](ENGINE_SOURCE_CHANGE.md)

## 中文说明

### 目的

`Frame Render Inspector` 需要从 `FRDGBuilder` 读取当前帧的 RDG 资源所有权映射，用于枚举和检查：

- pooled textures
- external textures
- pooled buffers
- external buffers

这些接口仅用于调试访问，不改变正常渲染逻辑。

### 修改文件

需要在以下引擎头文件中加入调试访问接口：

```text
Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h
```

### 建议加入的接口

```cpp
// --- DEBUG ACCESSORS ---

const TMap<FRDGPooledTexture*, FRDGTexture*, FRDGSetAllocator>& GetPooledTextureOwnershipMap() const
{
	return PooledTextureOwnershipMap;
}

const TSortedMap<FRHITexture*, FRDGTexture*, FRDGArrayAllocator>& GetExternalTextures() const
{
	return ExternalTextures;
}

const TMap<FRDGPooledBuffer*, FRDGBuffer*, FRDGSetAllocator>& GetPooledBufferOwnershipMap() const
{
	return PooledBufferOwnershipMap;
}

const TSortedMap<FRHIBuffer*, FRDGBuffer*, FRDGArrayAllocator>& GetExternalBuffers() const
{
	return ExternalBuffers;
}
```

### 用途说明

- `GetPooledTextureOwnershipMap()`
  - 用于枚举 RDG 中由 pooled texture 持有的纹理对象

- `GetExternalTextures()`
  - 用于枚举接入 RDG 的外部纹理对象

- `GetPooledBufferOwnershipMap()`
  - 用于枚举 RDG 中由 pooled buffer 持有的缓冲区对象

- `GetExternalBuffers()`
  - 用于枚举接入 RDG 的外部缓冲区对象

### 在本插件中的作用

这些接口会被 `Frame Render Inspector` 用来：

- 收集当前帧可检查的 RDG texture / buffer
- 将资源名称展示到调试面板
- 支持纹理预览、buffer 读回和资源筛选

### 注意事项

- 这是一个调试用途的引擎改动
- 建议在源码中明确标注为 `DEBUG ACCESSORS`
- 如果你的引擎分支已经有等价接口，可以直接复用，不需要重复添加
