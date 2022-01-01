// 3阶伯恩斯坦基函数
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(
    invT * invT * invT,
    3 * t * invT * invT,
    3 * t * t * invT,
    t * t * t
    );
}

// 3阶伯恩斯坦基函数的导数
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(
    -3 * invT * invT,
    3 * invT * invT - 6 * t * invT,
    6 * t * invT - 3 * t * t,
    3 * t * t
    );
}


struct HullOut
{
    float3 PosL : POSITION;
};

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezPatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);

    sum += basisV.x * (basisU.x * bezPatch[0].PosL  + basisU.y * bezPatch[1].PosL  + basisU.z * bezPatch[2].PosL  + basisU.w * bezPatch[3].PosL);
    sum += basisV.y * (basisU.x * bezPatch[4].PosL  + basisU.y * bezPatch[5].PosL  + basisU.z * bezPatch[6].PosL  + basisU.w * bezPatch[7].PosL);
    sum += basisV.z * (basisU.x * bezPatch[8].PosL  + basisU.y * bezPatch[9].PosL  + basisU.z * bezPatch[10].PosL + basisU.w * bezPatch[11].PosL);
    sum += basisV.w * (basisU.x * bezPatch[12].PosL + basisU.y * bezPatch[13].PosL + basisU.z * bezPatch[14].PosL + basisU.w * bezPatch[15].PosL);

    return sum;
}