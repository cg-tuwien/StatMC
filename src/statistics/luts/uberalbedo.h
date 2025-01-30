// © 2024-2025 Hiroyuki Sakai

#ifndef PBRT_STATISTICS_LUTS_UBERALBEDO_H
#define PBRT_STATISTICS_LUTS_UBERALBEDO_H

// statistics/luts/uberalbedo.h*
#include "pbrt.h"

namespace pbrt {

extern Float         uberAlbedoLUT[8*8*8*8*8*8*8*8];
extern unsigned char uberAlbedoLUTNDims;
extern unsigned char uberAlbedoLUTMaxIndices[8];
extern unsigned int  uberAlbedoLUTOffsets[256];

} // namespace pbrt

#endif // PBRT_STATISTICS_LUTS_UBERALBEDO_H
