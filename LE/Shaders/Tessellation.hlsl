#include "LightingUtil.hlsl"

#include "Common.hlsl"

Texture2D gDiffuseMap : register(t0);

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
PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PRIMITIVEID)
{
    // 首先求出四个控制点的中点
    float3 centerL = 0.25f * (
    patch[0].PosL +
    patch[1].PosL +
    patch[2].PosL +
    patch[3].PosL
    );

    float3 centerW = mul(float4(centerL, 1.0f), gWorld);
    float d = distance(centerW, gEyePosW);

    const float near = 10.0f;
    const float far = 80.0f;
    float tess = 64.0f * saturate((far - d) / (far - near));

    PatchTess pt;
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;

    return pt;
}

struct HullOut
{
    float3 PosL : POSITION;
};

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
[outputcontrolpoints(4)]
// 常量外壳着色器函数名称
[patchconstantfunc("ConstantHS")]
// 所用曲面细分因子的最大值
[maxtessfactor(64.0f)]
// 在此作为传递着色器
// 输入：面片的所有控制点
// 输出：同样是控制点
HullOut HS(InputPatch<VertexOut, 4> p,
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
const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;

    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);

    p.y = 0.3 * (p.z * sin(p.x) + p.x * cos(p.z)) + 5.0f;

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);

    return dout;
}

float4 PS(DomainOut pin) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}