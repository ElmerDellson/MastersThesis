// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 bary;
};

// #DXR Extra: Another ray type
struct ShadowHitInfo
{
    bool isHit;
};

struct STriVertex
{
    float3 vertex;
    float4 color;
};

// #DXR Extra: Simple lighting
struct InstanceProperties
{
    float4x4 objectToWorld;
    float4x4 objectToWorldNormal;
};

struct Surfel
{
    float3 color;
    float3 position;
    float3 normal;
};

struct Probe
{
    float3 colors[8];
};

bool VectorCompare(float3 in1, float3 in2)
{
    return (in1.x == in2.x && in1.y == in2.y && in1.z == in2.z);
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff)
{
    uint v0 = val0, v1 = val1, s0 = 0;

	//[unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    
    return v0;
}

float TraceShadowRayInline(float3 hitWorldPos, float3 lightDir, RaytracingAccelerationStructure SceneBVH, uint frameCount,
    uint2 randomSeed1, uint2 randomSeed2)
{
    int nbSamples = 1;
    uint randSeed = initRand(randomSeed1.x + randomSeed1.y * randomSeed2.x, frameCount, 16);
    float randFactor = 0;

    RayQuery<RAY_FLAG_NONE> q;
    RayDesc ray;
    ray.Origin = hitWorldPos;
    ray.TMin = 0.001;
    ray.TMax = 10000;

    float shadowFactor = 0;
    for (int i = 0; i < nbSamples; i++)
    {
        ray.Direction = float3(lightDir.x + nextRand(randSeed) * randFactor, 
                               lightDir.y + nextRand(randSeed) * randFactor,
                               lightDir.z + nextRand(randSeed) * randFactor);

        q.TraceRayInline(SceneBVH, RAY_FLAG_NONE, 0x01 /*cubes only*/, ray);
        q.Proceed();

        if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            shadowFactor += 1 / (float)nbSamples;
    }

    return 1 - shadowFactor;
}

float CalculateShadows(float3 normal, float3 hitWorldPos, float3 lightDir, RaytracingAccelerationStructure SceneBVH,
                       uint frameCount, uint2 randomSeed1, uint2 randomSeed2)
{
    bool isShadowed = dot(normal, lightDir) < 0.f;
    
    float shadowFactor = 0;
    // Trace shadow ray if we are not already in shadow.
    if (!isShadowed)
        shadowFactor = TraceShadowRayInline(hitWorldPos, lightDir, SceneBVH, frameCount, randomSeed1, randomSeed2);
    
    return shadowFactor;
}

float3 calculateAdvancedLighting(float3 hitWorldPos, float3 normal, float3 lightPos, float3 lightColor, bool useShadows, 
    float ambientTerm, RaytracingAccelerationStructure SceneBVH, uint frameCount, float lightSpread, 
    bool useDirectionalLight, float3 directionalLightAngle, uint2 randomSeed1, uint2 randomSeed2)
{
    float3 lightDir = useDirectionalLight ? normalize(directionalLightAngle) : normalize(lightPos - hitWorldPos);

    float shadowFactor = 1.0;
    if (useShadows)
        shadowFactor = CalculateShadows(normal, hitWorldPos, lightDir, SceneBVH, frameCount, randomSeed1, randomSeed2);
    
    float nDotL = clamp(dot(normal, lightDir), 0, 1); // Should normalize vectors, but it looks better if we don't
    float3 lightContrib = lightColor * nDotL;

    float lightDist = distance(hitWorldPos, lightPos);
    
    float distanceScaling = useDirectionalLight ? 1 : clamp(1 / (lightDist * lightDist / lightSpread), 0, 1);
    return max(ambientTerm, lightContrib * shadowFactor * distanceScaling);
}

float3 calculateProbeLightingOctagonal(float3 hitWorldPos, float3 lightPos, int nbProbesPerLayer, int nbProbeLayers, float basePlaneSideLength,
    bool interpolateProbes, StructuredBuffer<Probe> probes, float3 normal)
{
    float stepSize = basePlaneSideLength / (sqrt(nbProbesPerLayer) - 1);
    float x = min(max(hitWorldPos.x + basePlaneSideLength / 2, 0), basePlaneSideLength); //< actual x coordinate, but moved by maxX and clamped to [0, side].
    float z = min(max(hitWorldPos.z + basePlaneSideLength / 2, 0), basePlaneSideLength); //< actual z coordinate, but moved by maxZ and clamped to [0, side].
    float y = clamp(hitWorldPos.y, 0, nbProbeLayers * stepSize); //< y coordinate clamped to [0, nbProbeLayers * stepSize].

    float xSnappedDown = x - (x % stepSize); //< x rounded down to nearest stepSize.
    float zSnappedDown = z - (z % stepSize); //< z rounded down to nearest stepSize.
    float ySnappedDown = y - (y % stepSize); //< y rounded down to nearest stepSize.

    int xIndexDown = ((xSnappedDown) / stepSize) * sqrt(nbProbesPerLayer);
    int xIndexUp = ((xSnappedDown) / stepSize + 1) * sqrt(nbProbesPerLayer);
    int zIndexDown = (zSnappedDown) / stepSize;
    int zIndexUp = (zSnappedDown) / stepSize + 1;
    int yIndexDown = ((ySnappedDown) / stepSize) * nbProbesPerLayer;
    int yIndexUp = ((ySnappedDown) / stepSize + 1) * nbProbesPerLayer;
    
    float3 totalLightDownDownDown = 0, totalLightUpDownDown = 0, totalLightDownUpDown = 0, totalLightUpUpDown = 0, totalLightDownDownUp = 0, 
    totalLightUpDownUp = 0, totalLightDownUpUp = 0, totalLightUpUpUp = 0;

    float3 cardinalDirections[8] = { float3(1, 1, 0), float3(0, 1, 1), float3(-1, 1, 0), float3(0, 1, -1),
                                     float3(1, -1, 0), float3(0, -1, 1), float3(-1, -1, 0), float3(0, -1, -1) };
    
    for (int direction = 0; direction < 8; direction++)
    {
        float3 lightDir = cardinalDirections[direction];
        float nDotL = max(0.f, dot(normal, lightDir));
        
        totalLightDownDownDown += probes[xIndexDown + zIndexDown + yIndexDown].colors[direction] * nDotL;
        totalLightUpDownDown += probes[xIndexUp + zIndexDown + yIndexDown].colors[direction] * nDotL;
        totalLightDownUpDown += probes[xIndexDown + zIndexUp + yIndexDown].colors[direction] * nDotL;
        totalLightUpUpDown += probes[xIndexUp + zIndexUp + yIndexDown].colors[direction] * nDotL;
        totalLightDownDownUp += probes[xIndexDown + zIndexDown + yIndexUp].colors[direction] * nDotL;
        totalLightUpDownUp += probes[xIndexUp + zIndexDown + yIndexUp].colors[direction] * nDotL;
        totalLightDownUpUp += probes[xIndexDown + zIndexUp + yIndexUp].colors[direction] * nDotL;
        totalLightUpUpUp += probes[xIndexUp + zIndexUp + yIndexUp].colors[direction] * nDotL;
    }
    
    float3 lerpXColorDown = lerp(totalLightDownDownDown, totalLightUpDownDown, (x - xSnappedDown) / stepSize);
    float3 lerpZColorDown = lerp(totalLightDownUpDown, totalLightUpUpDown, (x - xSnappedDown) / stepSize);
    float3 downColor = lerp(lerpXColorDown, lerpZColorDown, (z - zSnappedDown) / stepSize);

    float3 lerpXColorUp = lerp(totalLightDownDownUp, totalLightUpDownUp, (x - xSnappedDown) / stepSize);
    float3 lerpZColorUp = lerp(totalLightDownUpUp, totalLightUpUpUp, (x - xSnappedDown) / stepSize);
    float3 upColor = lerp(lerpXColorUp, lerpZColorUp, (z - zSnappedDown) / stepSize);
    
    return lerp(downColor, upColor, (y - ySnappedDown) / stepSize);
}

// float TraceShadowRay(float3 hitWorldPos, float3 lightDir, RaytracingAccelerationStructure SceneBVH, int frameCount)
// {
//     uint2 randomSeed1 = DispatchRaysIndex().xy;
//     uint2 randomSeed2 = DispatchRaysDimensions().xy;

//     int nbSamples = 1;
//     uint randSeed = initRand(randomSeed1.x + randomSeed1.y * randomSeed2.x, frameCount, 16);
//     float randFactor = 0;

//     float shadowFactor = 0;
//     for (int i = 0; i < nbSamples; i++)
//     {
//         RayDesc ray;
//         ray.Origin = hitWorldPos;
//         ray.Direction = float3(lightDir.x + nextRand(randSeed) * randFactor, lightDir.y + nextRand(randSeed) * randFactor,
//             lightDir.z + nextRand(randSeed) * randFactor);
//         ray.TMin = 0.01;
//         ray.TMax = 100000;

//         // Initialize the ray payload.
//         ShadowHitInfo shadowPayload;
//         shadowPayload.isHit = false;

//         // Trace the ray.
//         TraceRay(
//                 // Acceleration structure
//                 SceneBVH,
//                 // Flags can be used to specify the behavior upon hitting a surface.
//                 RAY_FLAG_NONE,
//                 // Instance inclusion mask, which can be used to mask out some geometry to this ray by and-ing the mask with a
//                 // geometry mask. The 0xFF flag then indicates no geometry will be masked.
//                 0x01,
//                 // Depending on the type of ray, a given object can have several hit groups attached (i.e. what to do when
//                 // hitting to compute regular shading, and what to do when hitting to compute shadows). Those hit groups are
//                 // specified sequentially in the SBT, so the value below indicates which offset (on 4 bits) to apply to the hit
//                 // groups for this ray.
//                 1,
//                 // The offsets in the SBT can be computed from the object ID, its instance ID, but also simply by the order the
//                 // objects have been pushed in the acceleration structure. This allows the application to group shaders in the
//                 // SBT in the same order as they are added in the AS, in which case the value below represents the stride
//                 // (4 bits representing the number of hit groups) between two consecutive objects.
//                 0,
//                 // Index of the miss shader to use in case several consecutive miss shaders are present in the SBT. This allows
//                 // to change the behavior of the program when no geometry has been hit, for example to return a sky color for
//                 // regular rendering, and another returning a full visibility value for shadow rays.
//                 1,
//                 // Ray information to trace.
//                 ray,
//                 // Payload associated to the ray, which will be used to communicate between the hit/miss shaders and the raygen.
//                 shadowPayload
//             );

//         shadowFactor += shadowPayload.isHit ? 1 : 0;
//     }

//     return 1 - (shadowFactor / nbSamples);
// }

// float3 trilinearInterpolation(float x, float z, float y, float xSnappedDown, float zSnappedDown, float ySnappedDown,
//     float stepSize, int nbProbesPerLayer, StructuredBuffer<Probe> probes)
// {
//     int xIndexDown = ((xSnappedDown) / stepSize) * sqrt(nbProbesPerLayer);
//     int xIndexUp = ((xSnappedDown) / stepSize + 1) * sqrt(nbProbesPerLayer);
//     int zIndexDown = (zSnappedDown) / stepSize;
//     int zIndexUp = (zSnappedDown) / stepSize + 1;
//     int yIndexDown = ((ySnappedDown) / stepSize) * nbProbesPerLayer;
//     int yIndexUp = ((ySnappedDown) / stepSize + 1) * nbProbesPerLayer;

//     float3 colorDownDownDown = probes[xIndexDown + zIndexDown + yIndexDown].colors[0];
//     float3 colorUpDownDown = probes[xIndexUp + zIndexDown + yIndexDown].colors[0];
//     float3 colorDownUpDown = probes[xIndexDown + zIndexUp + yIndexDown].colors[0];
//     float3 colorUpUpDown = probes[xIndexUp + zIndexUp + yIndexDown].colors[0];
//     float3 colorDownDownUp = probes[xIndexDown + zIndexDown + yIndexUp].colors[0];
//     float3 colorUpDownUp = probes[xIndexUp + zIndexDown + yIndexUp].colors[0];
//     float3 colorDownUpUp = probes[xIndexDown + zIndexUp + yIndexUp].colors[0];
//     float3 colorUpUpUp = probes[xIndexUp + zIndexUp + yIndexUp].colors[0];

//     float3 lerpXColorDown = lerp(colorDownDownDown, colorUpDownDown, (x - xSnappedDown) / stepSize);
//     float3 lerpZColorDown = lerp(colorDownUpDown, colorUpUpDown, (x - xSnappedDown) / stepSize);
//     float3 downColor = lerp(lerpXColorDown, lerpZColorDown, (z - zSnappedDown) / stepSize);

//     float3 lerpXColorUp = lerp(colorDownDownUp, colorUpDownUp, (x - xSnappedDown) / stepSize);
//     float3 lerpZColorUp = lerp(colorDownUpUp, colorUpUpUp, (x - xSnappedDown) / stepSize);
//     float3 upColor = lerp(lerpXColorUp, lerpZColorUp, (z - zSnappedDown) / stepSize);

//     return lerp(downColor, upColor, (y - ySnappedDown) / stepSize);
// }