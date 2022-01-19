#include "Common.hlsl"

Texture2D gDiffuseMap[5] : register(t0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_INSTANCEID)
{
    VertexOut vout = (VertexOut)0.0f;

    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;

    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosH = mul(posW, gViewProj);

    MaterialData matData = gMaterialData[instData.MaterialIndex];
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), matData.MatTransform);
    vout.TexC = texC.xy;
    
    vout.MatIndex = instData.MaterialIndex;
    return vout;
}

void PS(VertexOut pin)
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    float4 diffuseAlbeo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;

    diffuseAlbeo *= gDiffuseMap[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

    #ifdef ALPHA_TEST
        clip(diffuseAlbeo.a - 0.1f);
    #endif
}