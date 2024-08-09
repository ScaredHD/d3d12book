cbuffer cbPerObject : register(b0) {
    float4x4 gModelToWolrd;
};

cbuffer cbPass : register(b1) {
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct VertexIn {
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VertexOut {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

VertexOut VS(VertexIn v) {
    VertexOut vout;

    float4 posW = mul(float4(v.pos, 1.0f), gModelToWolrd);
    vout.pos = mul(posW, gViewProj);
    vout.color = v.color;

    return vout;
}

float4 PS(VertexOut v) : SV_TARGET {
    return v.color;
}
