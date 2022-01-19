#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#include "Common.hlsl"

Texture2D gDiffuseMap[5] : register(t0);
Texture2D gShadowMap : register(t5);

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
    float4 ShadowPosH : POSITION0;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vertIn, uint instanceID : SV_INSTANCEID)
{
    VertexOut vertOut;

    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;
    vertOut.MatIndex = matIndex;

    float4 posW = mul(float4(vertIn.PosL, 1.0f), world);
    vertOut.PosH = mul(posW, gViewProj);
    vertOut.PosW = posW.xyz;
    MaterialData matData = gMaterialData[matIndex];
    float4 texC = mul(float4(vertIn.Texcoord, 0.0f, 1.0f), matData.MatTransform);
    vertOut.TexC = texC.xy;
    vertOut.NormalW = mul(vertIn.NormalL, (float3x3)world);
    vertOut.ShadowPosH = mul(posW, gShadowTransform);
    return vertOut;
}

float4 PS(VertexOut vertIn) : SV_Target
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[vertIn.MatIndex];
    float4 diffuseAlbedoMat = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    float4 diffuseAlbedo = gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, vertIn.TexC) * diffuseAlbedoMat;
    // 法线
    float3 worldNormal = normalize(vertIn.NormalW);
    // 视点方向
    float3 viewDir = normalize(gEyePosW - vertIn.PosW);

    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(gShadowMap, gsamShadow, vertIn.ShadowPosH);
    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess};
    float4 directLight = ComputeLighting(gLights, mat, vertIn.PosW, worldNormal, viewDir, shadowFactor);

    // 环境光
    float4 ambientColor = gAmbientLight * diffuseAlbedo;
    // 最终颜色
    float4 litColor = ambientColor + directLight;
    litColor.a = diffuseAlbedo.a;
    return litColor;
}