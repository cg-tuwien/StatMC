// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_MATTEALBEDO_H
#define PBRT_STATISTICS_LUTS_MATTEALBEDO_H

// statistics/luts/mattealbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         matteAlbedoLUT[8*8];
extern unsigned char matteAlbedoLUTNDims;
extern unsigned char matteAlbedoLUTMaxIndices[2];
extern unsigned int  matteAlbedoLUTOffsets[4];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_MATTEALBEDO_H
