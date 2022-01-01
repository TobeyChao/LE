#include "LightingUtil.hlsl"
#include "BezierUtil.hlsl"

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

SamplerState gsamPointWrap  : register(s0);
SamplerState gsamPointClamp  : register(s0);
SamplerState gsamLinearWrap  : register(s0);
SamplerState gsamLinearClamp  : register(s0);
SamplerState gsamAnisotropicWrap  : register(s0);
SamplerState gsamAnisotropicClamp  : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vertOut;
    vertOut.PosL = vin.PosL;
    return vertOut;
}

struct PatchTess
{
    // 镶嵌因子
    float EdgeTess[4] : SV_TESSFACTOR;
    // 内部镶嵌因子
    float InsideTess[2] : SV_INSIDETESSFACTOR;
    // Other Info

};

// 常量外壳着色器
PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PRIMITIVEID)
{
    // 首先求出四个控制点的中点
    float3 centerL = 0.25f * (
    patch[0].PosL +
    patch[3].PosL +
    patch[12].PosL +
    patch[15].PosL
    );

    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
    float d = distance(centerW, gEyePosW);

    const float near = 10.0f;
    const float far = 80.0f;
    const float maxtessfactor = 32.0f;
    float tess = maxtessfactor * saturate((far - d) / (far - near));

    PatchTess pt;
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;

    return pt;
}

// 面片的类型
// 1.tri 三角形面片
// 2.quad 四边形面片
[domain("quad")]
// 细分模式
// 1.integer
// 非整形曲面细分
// 2.fractional_even/fractional_odd
[partitioning("integer")]
// 输出三角形绕序
// 1.triangle_cw
// 2.triangle_ccw
// 3.line 线段曲面细分
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
// 常量外壳着色器函数名称
[patchconstantfunc("ConstantHS")]
// 所用曲面细分因子的最大值
[maxtessfactor(32.0f)]
// 在此作为传递着色器
// 输入：面片的所有控制点
// 输出：同样是控制点
HullOut HS(InputPatch<VertexOut, 16> p,
uint i : SV_OUTPUTCONTROLPOINTID,
uint patchId : SV_PRIMITIVEID)
{
    HullOut hout;
    hout.PosL = p[i].PosL;
    return hout;
}

struct DomainOut
{
    float4 PosH : SV_POSITION;
};

[domain("quad")]
// 域着色器
// 镶嵌器阶段输出新建的所有顶点和三角形,镶嵌器阶段创建的顶点都会逐一调用域着色器
// 可以把它看作镶嵌处理阶段后的顶点着色器
DomainOut DS(PatchTess patchTess,
float2 uv : SV_DOMAINLOCATION,
const OutputPatch<HullOut, 16> bzPatch)
{
    DomainOut dout;

    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);

    float3 p = CubicBezierSum(bzPatch, basisU, basisV) + float3(0.0f, 4.0f, 0.0f);

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);

    return dout;
}

float4 PS(DomainOut pin) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}