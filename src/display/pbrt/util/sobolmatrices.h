// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

/*
    This file contains modifications to the original pbrt source code for the
    paper "A Statistical Approach to Monte Carlo Denoising"
    (https://www.cg.tuwien.ac.at/StatMC).
    
    Copyright © 2024-2025 Hiroyuki Sakai for the modifications.
    Original copyright and license (refer to the top of the file) remain
    unaffected.
 */

#ifndef PBRT_UTIL_SOBOLMATRICES_H
#define PBRT_UTIL_SOBOLMATRICES_H

#include <pbrt/pbrt.h>

#include <cstdint>

namespace pbrtv4 {

// Sobol Matrix Declarations
static constexpr int NSobolDimensions = 1024;
static constexpr int SobolMatrixSize = 52;
extern PBRT_CONST uint32_t SobolMatrices32[NSobolDimensions * SobolMatrixSize];

extern PBRT_CONST uint64_t VdCSobolMatrices[][SobolMatrixSize];
extern PBRT_CONST uint64_t VdCSobolMatricesInv[][SobolMatrixSize];

}  // namespace pbrtv4

#endif  // PBRT_UTIL_SOBOLMATRICES_H
