#pragma once

#include "Common/d3dUtil.h"

struct RenderItem {
    DirectX::XMFLOAT4X4 modelToWorld;

    int dirtyFrameCount = 3;

    UINT objectCbufferIndex = -1;

    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT indexCount;
    UINT indexStart;
    UINT baseVertex;
};
