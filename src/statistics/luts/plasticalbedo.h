// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_PLASTICALBEDO_H
#define PBRT_STATISTICS_LUTS_PLASTICALBEDO_H

// statistics/luts/plasticalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         plasticAlbedoLUT[8*8*8*8];
extern unsigned char plasticAlbedoLUTNDims;
extern unsigned char plasticAlbedoLUTMaxIndices[4];
extern unsigned int  plasticAlbedoLUTOffsets[16];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_PLASTICALBEDO_H
