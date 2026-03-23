#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FRDGBuffer;

void EnumerateRDGBuffers(FRDGBuilder& GraphBuilder, TFunctionRef<void(FRDGBuffer* Buffer)> Callback);
