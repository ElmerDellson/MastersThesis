#include "DataTypes.hlsl"

#ifndef UTILITY
#define UTILITY

static const float pi = 3.14159265358979;

bool VectorCompare(float3 in1, float3 in2)
{
//    return (in1.x == in2.x && in1.y == in2.y && in1.z == in2.z);
    return all(abs(in1 - in2) < 0.0001f);
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

bool CheckAABB(float3 p, AABB AABBIn, float bias)
{
    return (p.x < AABBIn.boundaries[0] + bias && p.x > AABBIn.boundaries[1] - bias) &&
           (p.y < AABBIn.boundaries[2] + bias && p.y > AABBIn.boundaries[3] - bias) &&
           (p.z < AABBIn.boundaries[4] + bias && p.z > AABBIn.boundaries[5] - bias);
}

float FindSurfelDistLimit(float3 direction, float distBetweenProbes)
{
    float3 normalizedDirection = normalize(direction);
    float longestComponent = max(max(abs(normalizedDirection.x), abs(normalizedDirection.y)), abs(normalizedDirection.z));

    float3 directionWithLargestComponentLengthOne = normalizedDirection / longestComponent;
    float3 directionWithLargestComponentLengthDistBetweenProbes = directionWithLargestComponentLengthOne * distBetweenProbes;

    return length(directionWithLargestComponentLengthDistBetweenProbes);
}

#endif