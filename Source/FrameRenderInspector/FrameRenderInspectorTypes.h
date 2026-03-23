#pragma once

#include "CoreMinimal.h"

struct FTextureDebuggerItem
{
	FString Name;
	FIntPoint Size;
	FString Format;
};

struct FBufferDebuggerItem
{
	FString Name;
	uint32 Stride = 0;
	uint32 Count = 0;
	uint64 NumBytes = 0;

	bool operator==(const FBufferDebuggerItem& Other) const
	{
		return Name == Other.Name && Stride == Other.Stride && Count == Other.Count && NumBytes == Other.NumBytes;
	}
};

struct FBufferReadbackResult
{
	FString Name;
	uint32 Stride = 0;
	uint32 Count = 0;
	TArray<uint8> Data;
	FString StatusMessage;
	bool bSucceeded = false;
};

enum class ERenderOptionValueType : uint8
{
	Bool,
	Int,
	Float
};

struct FRenderOptionItem
{
	FString Name;
	ERenderOptionValueType ValueType = ERenderOptionValueType::Bool;
	bool bBoolValue = false;
	FString ValueText;
};
