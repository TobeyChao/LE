#include "LightingUtil.hlsl"

#include "Common.hlsl"

Texture2D gDiffuseMap[4] : register(t0);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gsamPointWrap  : register(s0);
SamplerState gsamPointClamp  : register(s0);
SamplerState gsamLinearWrap  : register(s0);
SamplerState gsamLinearClamp  : register(s0);
SamplerState gsamAnisotropicWrap  : register(s0);
SamplerState gsamAnisotropicClamp  : register(s0);

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
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vertIn)
{
    VertexOut vertOut;
    float4 posW = mul(float4(vertIn.PosL, 1.0f), gWorld);
    vertOut.PosH = mul(posW, gViewProj);
    vertOut.PosW = posW.xyz;
	MaterialData matData = gMaterialData[gMaterialIndex];
    float4 texC = mul(float4(vertIn.Texcoord, 0.0f, 1.0f), matData.MatTransform);
    vertOut.TexC = texC.xy;
    vertOut.NormalW = mul(vertIn.NormalL, (float3x3)gWorld);
    return vertOut;
}

float4 PS(VertexOut vertIn) : SV_Target
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedoMat = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;

    // 法线
	float3 worldNormal = normalize(vertIn.NormalW);
    // 灯光方向
	float3 worldLightDir = normalize(-gLights[0].Direction);
    // 视点方向
	float3 viewDir = normalize(gEyePosW - vertIn.PosW);
    // 朗伯余弦定律
	float cosIncidentAngle = saturate(dot(worldLightDir, worldNormal));
    // 光强
    float3 lightStrength = gLights[0].Strength.rgb * cosIncidentAngle;
    // 1.漫反射光
    float4 diffuseAlbedo = gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, vertIn.TexC) * diffuseAlbedoMat;
	float3 diffuseColor = lightStrength * diffuseAlbedo.rgb;
    // 2.镜面光
	// 2.1表面粗糙度
	float3 halfDir =  normalize(viewDir + worldLightDir);
    float m = (1.0f - roughness) * 256.0f;
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfDir, worldNormal), 0.0f), m) / 8.0f;
    // 2.2菲涅尔系数
    float f0 = 1.0f - cosIncidentAngle;
    float3 fresnelFactor = fresnelR0 + (1.0f - fresnelR0)*(f0*f0*f0*f0*f0);
    // 最终镜面光颜色
	float3 specularColor = lightStrength * fresnelFactor * roughnessFactor;
    // 3.环境光
    float4 ambientColor = gAmbientLight * diffuseAlbedo;
    // 最终颜色
	float3 color = ambientColor.rgb + diffuseColor + specularColor;
	return float4(color, diffuseAlbedo.a);
}