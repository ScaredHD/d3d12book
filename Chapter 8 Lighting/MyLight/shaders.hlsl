#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif
#ifndef NUM_POINT_LIGHT
#define NUM_POINT_LIGHT 0
#endif
#ifndef NUM_SPOT_LIGHT
#define NUM_SPOT_LIGHT 0
#endif

#include "LightingUtil.hlsl"

cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
};

cbuffer cbMaterial : register(b1) {
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float gRoughness;
};

cbuffer cbPass : register(b2) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float3 gEyePosW;
  float cbPerObjectPad1;
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
  float gTotalTime;
  float gDeltaTime;
  float4 gAmbientLight;

  Light gLights[MaxLights];
};

struct VertexIn {
  float3 PosL : POSITION;
  float3 NormalL : NORMAL;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float3 PosW : POSITION;
  float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin) {
  VertexOut vout;

  float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
  vout.PosW = posW.xyz;

  vout.NormalW = mul(gWorld, vin.NormalL);

  vout.PosH = mul(mul(gProj, gView), posW);

  return vout;
}

float4 PS(VertexOut pin) : SV_Target {
  pin.NormalW = normalize(pin.NormalW);

  float3 toEyeW = normalize(gEyePosW - pin.PosW);

  float4 ambient = gAmbientLight * gDiffuseAlbedo;

  const float shininess = 1.0f - gRoughness;
  Material mat = {gDiffuseAlbedo, gFresnelR0, shininess};
  float3 shadowFactor = 1.0f;
  float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

  float4 litColor = ambient + directLight;

  litColor.a = gDiffuseAlbedo.a;
  return litColor;
}
