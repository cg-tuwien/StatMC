// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_GLASSALBEDO_H
#define PBRT_STATISTICS_LUTS_GLASSALBEDO_H

// statistics/luts/glassalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         glassAlbedoLUT[8*8*8*8*8*8];
extern unsigned char glassAlbedoLUTNDims;
extern unsigned char glassAlbedoLUTMaxIndices[6];
extern unsigned int  glassAlbedoLUTOffsets[64];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_GLASSALBEDO_H
