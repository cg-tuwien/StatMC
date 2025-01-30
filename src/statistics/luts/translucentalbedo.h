// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_TRANSLUCENTALBEDO_H
#define PBRT_STATISTICS_LUTS_TRANSLUCENTALBEDO_H

// statistics/luts/translucentalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         translucentAlbedoLUT[8*8*8*8*8*8];
extern unsigned char translucentAlbedoLUTNDims;
extern unsigned char translucentAlbedoLUTMaxIndices[6];
extern unsigned int  translucentAlbedoLUTOffsets[64];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_TRANSLUCENTALBEDO_H
