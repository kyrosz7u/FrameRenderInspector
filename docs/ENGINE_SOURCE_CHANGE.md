# Engine Source Change

[中文说明](ENGINE_SOURCE_CHANGE_ZH.md)

## Purpose

`Frame Render Inspector` needs access to RDG ownership maps from `FRDGBuilder` in order to enumerate and inspect:

- pooled textures
- external textures
- pooled buffers
- external buffers

These accessors are for debug inspection only and do not change normal rendering behavior.

## Modified File

Add the debug accessors to the following engine header:

```text
Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h
```

## Suggested Accessors

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

## What They Are Used For

- `GetPooledTextureOwnershipMap()`
  - enumerates RDG textures backed by pooled textures

- `GetExternalTextures()`
  - enumerates textures imported into RDG as external resources

- `GetPooledBufferOwnershipMap()`
  - enumerates RDG buffers backed by pooled buffers

- `GetExternalBuffers()`
  - enumerates buffers imported into RDG as external resources

## How This Plugin Uses Them

`Frame Render Inspector` uses these accessors to:

- collect inspectable RDG textures and buffers for the current frame
- expose resource names in the debug UI
- support texture preview, buffer readback, and resource filtering

## Notes

- this is an engine-side debug-only modification
- it is recommended to label the block clearly as `DEBUG ACCESSORS`
- if your engine branch already exposes equivalent accessors, you can reuse them directly
