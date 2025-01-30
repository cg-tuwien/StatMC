// © 2024-2025 Hiroyuki Sakai

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_STATISTICS_LUT_H
#define PBRT_STATISTICS_LUT_H

#include "pbrt.h"
#include <algorithm> // std::min

namespace pbrt {

#define LUT_LERP(arrayPrefix, arraySuffix, dimensionIndex, i0, i1) \
    arrayPrefix i0 arraySuffix * d0[dimensionIndex] + \
    arrayPrefix i1 arraySuffix * d1[dimensionIndex]
#define LUT_LERP_1D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[  0] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,   0,   1);
#define LUT_LERP_2D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_1D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[  1] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,   2,   3);
#define LUT_LERP_3D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_2D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[  2] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,   4,   5); \
    c[  3] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,   6,   7);
#define LUT_LERP_4D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_3D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[  4] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,   8,   9); \
    c[  5] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  10,  11); \
    c[  6] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  12,  13); \
    c[  7] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  14,  15);
#define LUT_LERP_5D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_4D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[  8] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  16,  17); \
    c[  9] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  18,  19); \
    c[ 10] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  20,  21); \
    c[ 11] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  22,  23); \
    c[ 12] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  24,  25); \
    c[ 13] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  26,  27); \
    c[ 14] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  28,  29); \
    c[ 15] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  30,  31);
#define LUT_LERP_6D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_5D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[ 16] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  32,  33); \
    c[ 17] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  34,  35); \
    c[ 18] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  36,  37); \
    c[ 19] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  38,  39); \
    c[ 20] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  40,  41); \
    c[ 21] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  42,  43); \
    c[ 22] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  44,  45); \
    c[ 23] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  46,  47); \
    c[ 24] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  48,  49); \
    c[ 25] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  50,  51); \
    c[ 26] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  52,  53); \
    c[ 27] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  54,  55); \
    c[ 28] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  56,  57); \
    c[ 29] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  58,  59); \
    c[ 30] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  60,  61); \
    c[ 31] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  62,  63);
#define LUT_LERP_7D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_6D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[ 32] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  64,  65); \
    c[ 33] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  66,  67); \
    c[ 34] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  68,  69); \
    c[ 35] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  70,  71); \
    c[ 36] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  72,  73); \
    c[ 37] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  74,  75); \
    c[ 38] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  76,  77); \
    c[ 39] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  78,  79); \
    c[ 40] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  80,  81); \
    c[ 41] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  82,  83); \
    c[ 42] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  84,  85); \
    c[ 43] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  86,  87); \
    c[ 44] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  88,  89); \
    c[ 45] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  90,  91); \
    c[ 46] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  92,  93); \
    c[ 47] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  94,  95); \
    c[ 48] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  96,  97); \
    c[ 49] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex,  98,  99); \
    c[ 50] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 100, 101); \
    c[ 51] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 102, 103); \
    c[ 52] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 104, 105); \
    c[ 53] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 106, 107); \
    c[ 54] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 108, 109); \
    c[ 55] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 110, 111); \
    c[ 56] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 112, 113); \
    c[ 57] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 114, 115); \
    c[ 58] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 116, 117); \
    c[ 59] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 118, 119); \
    c[ 60] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 120, 121); \
    c[ 61] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 122, 123); \
    c[ 62] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 124, 125); \
    c[ 63] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 126, 127);
#define LUT_LERP_8D(arrayPrefix, arraySuffix, dimensionIndex) \
             LUT_LERP_7D(arrayPrefix, arraySuffix, dimensionIndex) \
    c[ 64] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 128, 129); \
    c[ 65] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 130, 131); \
    c[ 66] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 132, 133); \
    c[ 67] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 134, 135); \
    c[ 68] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 136, 137); \
    c[ 69] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 138, 139); \
    c[ 70] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 140, 141); \
    c[ 71] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 142, 143); \
    c[ 72] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 144, 145); \
    c[ 73] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 146, 147); \
    c[ 74] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 148, 149); \
    c[ 75] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 150, 151); \
    c[ 76] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 152, 153); \
    c[ 77] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 154, 155); \
    c[ 78] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 156, 157); \
    c[ 79] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 158, 159); \
    c[ 80] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 160, 161); \
    c[ 81] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 162, 163); \
    c[ 82] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 164, 165); \
    c[ 83] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 166, 167); \
    c[ 84] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 168, 169); \
    c[ 85] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 170, 171); \
    c[ 86] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 172, 173); \
    c[ 87] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 174, 175); \
    c[ 88] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 176, 177); \
    c[ 89] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 178, 179); \
    c[ 90] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 180, 181); \
    c[ 91] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 182, 183); \
    c[ 92] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 184, 185); \
    c[ 93] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 186, 187); \
    c[ 94] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 188, 189); \
    c[ 95] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 190, 191); \
    c[ 96] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 192, 193); \
    c[ 97] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 194, 195); \
    c[ 98] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 196, 197); \
    c[ 99] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 198, 199); \
    c[100] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 200, 201); \
    c[101] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 202, 203); \
    c[102] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 204, 205); \
    c[103] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 206, 207); \
    c[104] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 208, 209); \
    c[105] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 210, 211); \
    c[106] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 212, 213); \
    c[107] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 214, 215); \
    c[108] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 216, 217); \
    c[109] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 218, 219); \
    c[110] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 220, 221); \
    c[111] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 222, 223); \
    c[112] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 224, 225); \
    c[113] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 226, 227); \
    c[114] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 228, 229); \
    c[115] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 230, 231); \
    c[116] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 232, 233); \
    c[117] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 234, 235); \
    c[118] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 236, 237); \
    c[119] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 238, 239); \
    c[120] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 240, 241); \
    c[121] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 242, 243); \
    c[122] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 244, 245); \
    c[123] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 246, 247); \
    c[124] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 248, 249); \
    c[125] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 250, 251); \
    c[126] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 252, 253); \
    c[127] = LUT_LERP   (arrayPrefix, arraySuffix, dimensionIndex, 254, 255);

inline Float LookupTable(
    const Float *lut,
    const unsigned char dimensionCount,
    const unsigned char *maxIndices,
    const unsigned int *offsets,
    const Float *normalizedIndices
) {
    unsigned int lutIndex = 0;
    Float d0[dimensionCount], d1[dimensionCount];
    for (unsigned char i = 0; i < dimensionCount; i++) {
        const Float index = std::min(normalizedIndices[i] * maxIndices[i],
                                     (Float) maxIndices[i]);
        const unsigned char lowerIndex = std::min((int)index,
                                                 maxIndices[i] - 1);

        // Calculate deltas to nearest coefficients
        d1[i] = index - lowerIndex;
        d0[i] = 1.f - d1[i];

        // Calculate first index of hypercube
        lutIndex += lowerIndex * offsets[1 << i];
    }

    // Calculate value for dimensionCount <= 8
    // Using unrolled loops is usually faster than the general loop-based
    // code below (timings compared for gcc 4.8.4 and clang).
    switch (dimensionCount) {
        case 1: {
            return LUT_LERP   (lut[lutIndex + offsets[, ]], 0, 0, 1);
        }
        case 2: {
            Float c[2];
                   LUT_LERP_2D(lut[lutIndex + offsets[, ]], 0)
            return LUT_LERP_1D(c[, ]                      , 1)
        }
        case 3: {
            Float c[4];
                   LUT_LERP_3D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_2D(c[, ]                      , 1)
            return LUT_LERP_1D(c[, ]                      , 2)
        }
        case 4: {
            Float c[8];
                   LUT_LERP_4D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_3D(c[, ]                      , 1)
                   LUT_LERP_2D(c[, ]                      , 2)
            return LUT_LERP_1D(c[, ]                      , 3)
        }
        case 5: {
            Float c[16];
                   LUT_LERP_5D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_4D(c[, ]                      , 1)
                   LUT_LERP_3D(c[, ]                      , 2)
                   LUT_LERP_2D(c[, ]                      , 3)
            return LUT_LERP_1D(c[, ]                      , 4)
        }
        case 6: {
            Float c[32];
                   LUT_LERP_6D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_5D(c[, ]                      , 1)
                   LUT_LERP_4D(c[, ]                      , 2)
                   LUT_LERP_3D(c[, ]                      , 3)
                   LUT_LERP_2D(c[, ]                      , 4)
            return LUT_LERP_1D(c[, ]                      , 5)
        }
        case 7: {
            Float c[64];
                   LUT_LERP_7D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_6D(c[, ]                      , 1)
                   LUT_LERP_5D(c[, ]                      , 2)
                   LUT_LERP_4D(c[, ]                      , 3)
                   LUT_LERP_3D(c[, ]                      , 4)
                   LUT_LERP_2D(c[, ]                      , 5)
            return LUT_LERP_1D(c[, ]                      , 6)
        }
        case 8: {
            Float c[128];
                   LUT_LERP_8D(lut[lutIndex + offsets[, ]], 0)
                   LUT_LERP_7D(c[, ]                      , 1)
                   LUT_LERP_6D(c[, ]                      , 2)
                   LUT_LERP_5D(c[, ]                      , 3)
                   LUT_LERP_4D(c[, ]                      , 4)
                   LUT_LERP_3D(c[, ]                      , 5)
                   LUT_LERP_2D(c[, ]                      , 6)
            return LUT_LERP_1D(c[, ]                      , 7)
        }
    }

    if (dimensionCount == 1)
        return LUT_LERP(lut[lutIndex + offsets[, ]], 0, 0, 1);

    // Calculate value for dimensionCount > 8
    const unsigned int maxDimensionIndex = dimensionCount - 1;
    unsigned int coefficientCount = 1 << maxDimensionIndex; // Name maxDimensionIndex is a bit off here
    Float c[coefficientCount];

    // Read coefficients from LUT
    for (unsigned int i = 0, j = 0; i < coefficientCount; i++, j += 2)
        c[i] = LUT_LERP(lut[lutIndex + offsets[, ]], 0, j, j + 1);

    // Lerp along dimensions
    unsigned int k;
    for (k = 1; k < maxDimensionIndex; k++) {
        coefficientCount >>= 1;
        for (unsigned int i = 0, j = 0; i < coefficientCount; i++, j += 2)
            c[i] = LUT_LERP(c[, ], k, j, j + 1);
    }

    return LUT_LERP(c[, ], k, 0, 1);
}

#undef LUT_LERP
#undef LUT_LERP_1D
#undef LUT_LERP_2D
#undef LUT_LERP_3D
#undef LUT_LERP_4D
#undef LUT_LERP_5D
#undef LUT_LERP_6D
#undef LUT_LERP_7D
#undef LUT_LERP_8D

}  // namespace pbrt

#endif  // PBRT_STATISTICS_LUT_H
