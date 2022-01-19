#include "Common.hlsl"

TextureCube gCubeMap : register(t1);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 Texcoord : TEXCOORD;
    float3 NormalL : NORMAL;
    float4 Color : COLOR0;  // Vertex Color
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_INSTANCEID)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    InstanceData instData = gInstanceData[instanceID];
    float4 posW = mul(float4(vin.PosL, 1.0f), instData.World);
    posW.xyz += gEyePosW;
    vout.PosH = mul(posW, gViewProj).xyww;
    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}