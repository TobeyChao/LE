#include "LightingUtil.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
}

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
    float4x4 gMatTransform;
};

cbuffer cbPass : register(b2)
{
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
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

Texture2DArray gTreeMapArray : register(t0);
SamplerState gsamPointWrap  : register(s0);
SamplerState gsamPointClamp  : register(s0);
SamplerState gsamLinearWrap  : register(s0);
SamplerState gsamLinearClamp  : register(s0);
SamplerState gsamAnisotropicWrap  : register(s0);
SamplerState gsamAnisotropicClamp  : register(s0);

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
	float3 uvw = float3(vertIn.TexC, vertIn.PrimID % 3);
    float4 diffuseAlbedo = gTreeMapArray.Sample(gsamAnisotropicWrap, uvw) * gDiffuseAlbedo;
#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif

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
    float3 diffuseColor = lightStrength * diffuseAlbedo.rgb;
    // 2.镜面光
    // 2.1表面粗糙度
    float3 halfDir =  normalize(viewDir + worldLightDir);
    float m = (1.0f - gRoughness) * 256.0f;
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfDir, worldNormal), 0.0f), m) / 8.0f;
    // 2.2菲涅尔系数
    float f0 = 1.0f - cosIncidentAngle;
    float3 fresnelFactor = gFresnelR0 + (1.0f - gFresnelR0)*(f0*f0*f0*f0*f0);
    // 最终镜面光颜色
    float3 specularColor = lightStrength * fresnelFactor * roughnessFactor;
    // 3.环境光
    float4 ambientColor = gAmbientLight * diffuseAlbedo;
    // 最终颜色
    float3 color = ambientColor.rgb + diffuseColor + specularColor;
    return float4(color, diffuseAlbedo.a);
}