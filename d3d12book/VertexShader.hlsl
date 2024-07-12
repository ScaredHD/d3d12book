cbuffer cbPerObject : register(b0) {
    float4x4 gModelViewProj;
}

void VS(float3 pos: POSITION, 
        float4 color: COLOR, 
        out float4 outPos: SV_POSITION, 
        out float4 outColor: COLOR) {
    outPos = mul(float4(pos, 1.0f), gModelViewProj);
    outColor = color;
}