// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_METALALBEDO_H
#define PBRT_STATISTICS_LUTS_METALALBEDO_H

// statistics/luts/metalalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         metalAlbedoLUT[8*8*8*8*8];
extern unsigned char metalAlbedoLUTNDims;
extern unsigned char metalAlbedoLUTMaxIndices[5];
extern unsigned int  metalAlbedoLUTOffsets[32];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_METALALBEDO_H
