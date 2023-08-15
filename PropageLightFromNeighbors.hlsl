#include "Common.hlsl"

#ifndef PROPAGATE_NEIGHBORS
#define PROPAGATE_NEIGHBORS

ProbeArray Get8Neighborhood(uint3 probeIndicesClamped, uint nbProbesPerLayer, RWStructuredBuffer<Probe> probes)
{
    ProbeArray probeArray;

    Probe tempArray[8] = {
        probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z],
        probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1],
        probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z],
        probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1],
        probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z],
        probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1],
        probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z],
        probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1]
    };

    probeArray.probes = tempArray;

    return probeArray;
}

float3 PropagateLightFromNeighborsOctagonal(uint3 probeIndicesClamped, int direction, float distanceScaling,
                                            uint nbProbesPerLayer, RWStructuredBuffer<Probe> probes)
{
    // Propagate probe lighting by checking the color from direction of the neighboring probe in the current direction.
    switch (direction)
    {
    case 0: // Upper left.
        return probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[0] * distanceScaling;
    case 1: // Upper front.
        return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1].colors[1] * distanceScaling;
    case 2: // Upper right.
        return probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[2] * distanceScaling;
    case 3: // Upper back.
        return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y + 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1].colors[3] * distanceScaling;
    case 4: // Lower left.
        return probes[(probeIndicesClamped.x + 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[4] * distanceScaling;
    case 5: // Lower front.
        return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z + 1].colors[5] * distanceScaling;
    case 6: // Lower right.
        return probes[(probeIndicesClamped.x - 1) * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z].colors[6] * distanceScaling;
    default: // Lower back.
        return probes[probeIndicesClamped.x * (uint)sqrt(nbProbesPerLayer) + (probeIndicesClamped.y - 1) 
                    * nbProbesPerLayer + probeIndicesClamped.z - 1].colors[7] * distanceScaling;
    }
}

#endif