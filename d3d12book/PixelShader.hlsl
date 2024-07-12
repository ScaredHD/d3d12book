
struct VertexOut {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(VertexOut pin) : SV_Target {
    return pin.color;
}