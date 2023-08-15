#ifndef DATA_TYPES
#define DATA_TYPES

// Hit information, aka ray payload. This sample only carries a shading color and hit distance. Note that the payload 
// should be kept as small as possible, and that its size must be declared in the corresponding 
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface, here the barycentric coordinates.
struct Attributes
{
  float2 bary;
};

struct ShadowHitInfo
{
    bool isHit;
};

struct STriVertex
{
    float3 vertex;
    float4 color;
};

struct InstanceProperties
{
    float4x4 objectToWorld;
    float4x4 objectToWorldNormal;
};

struct Surfel
{
    float3 albedo;
    float3 position;
    float3 normal;
};

struct Probe
{
    float3 colors[8];
};

struct SurfelBatch64
{
    Surfel surfels[64];
};

struct AABB
{
    float boundaries[6];
};

struct ProbeArray
{
    Probe probes[8];
};

#endif