// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_SUBSTRATEALBEDO_H
#define PBRT_STATISTICS_LUTS_SUBSTRATEALBEDO_H

// statistics/luts/substratealbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         substrateAlbedoLUT[8*8*8*8*8];
extern unsigned char substrateAlbedoLUTNDims;
extern unsigned char substrateAlbedoLUTMaxIndices[5];
extern unsigned int  substrateAlbedoLUTOffsets[32];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_SUBSTRATEALBEDO_H
