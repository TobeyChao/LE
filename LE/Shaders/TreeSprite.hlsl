#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#include "Common.hlsl"

Texture2DArray gTreeMapArray : register(t0);
Texture2D gShadowMap : register(t1);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

struct VertexIn
{
    float3 PosW  : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW   : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    uint PrimID  : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vertOut;
    // float4 posW = mul(float4(vertIn.PosL, 1.0f), gWorld);
    // vertOut.PosH = mul(posW, gViewProj);
    // vertOut.PosW = posW.xyz;
    // float4 texC = mul(float4(vertIn.Texcoord, 0.0f, 1.0f), gMatTransform);
    // vertOut.TexC = texC.xy;
    // vertOut.NormalW = mul(vertIn.NormalL, (float3x3)gWorld);
    vertOut.CenterW = vin.PosW;
    vertOut.SizeW = vin.SizeW;
    return vertOut;
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1], uint primID : SV_PRIMITIVEID, inout TriangleStream<GeoOut> triStream)
{
    float3 up = float3(0, 1, 0);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0;
    look = normalize(look);
    float3 right = cross(up, look);

    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;

    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0);

    float2 texC[4] = 
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.TexC = texC[i];
        gout.PrimID = primID;

        triStream.Append(gout);
    }
}

float4 PS(GeoOut vertIn) : SV_Target
{
    MaterialData matData = gMaterialData[0];
    float4 diffuseAlbedoMat = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;

    float3 uvw = float3(vertIn.TexC, vertIn.PrimID % 3);
    float4 diffuseAlbedo = gTreeMapArray.Sample(gsamAnisotropicWrap, uvw) * diffuseAlbedoMat;
    #ifdef ALPHA_TEST
        // Discard pixel if texture alpha < 0.1.  We do this test as soon 
        // as possible in the shader so that we can potentially exit the
        // shader early, thereby skipping the rest of the shader code.
        clip(diffuseAlbedo.a - 0.1f);
    #endif

    // 法线
    float3 worldNormal = normalize(vertIn.NormalW);
    // 视点方向
    float3 viewDir = normalize(gEyePosW - vertIn.PosW);

    float3 shadowFactor = 1.0f;
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