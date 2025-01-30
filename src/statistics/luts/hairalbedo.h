// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_HAIRALBEDO_H
#define PBRT_STATISTICS_LUTS_HAIRALBEDO_H

// statistics/luts/hairalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         hairAlbedoLUT[8*8*8*8];
extern unsigned char hairAlbedoLUTNDims;
extern unsigned char hairAlbedoLUTMaxIndices[4];
extern unsigned int  hairAlbedoLUTOffsets[16];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_HAIRALBEDO_H
