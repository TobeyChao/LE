cbuffer cbPerObject : register(b0)
{
    float4x4 gMatrixWVP;
}

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR0;
};
            
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR0;
};

VertexOut VS(VertexIn vertIn)
{
    VertexOut vertOut;
    vertOut.PosH = mul(float4(vertIn.PosL, 1.0f), gMatrixWVP);
    vertOut.Color = vertIn.Color;
    return vertOut;
}

float4 PS(VertexOut vertIn) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}