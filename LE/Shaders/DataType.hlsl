struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

struct MaterialData
{
    float4   DiffuseAlbedo;
    float3   FresnelR0;
    float    Roughness;
    float4x4 MatTransform;
    uint     DiffuseMapIndex;
    uint     MatPad0;
    uint     MatPad1;
    uint     MatPad2;
};

#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    uint     MaterialIndex;
    uint     InstPad0;
    uint     InstPad1;
    uint     InstPad2;
};