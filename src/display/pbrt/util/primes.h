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

#ifndef PBRT_UTIL_PRIMES_H
#define PBRT_UTIL_PRIMES_H

#include <pbrt/pbrt.h>

#include <pbrt/util/pstd.h>

namespace pbrtv4 {

// Prime Table Declarations
static constexpr int PrimeTableSize = 1000;
extern PBRT_CONST int Primes[PrimeTableSize];

}  // namespace pbrtv4

#endif  // PBRT_UTIL_PRIMES_H
