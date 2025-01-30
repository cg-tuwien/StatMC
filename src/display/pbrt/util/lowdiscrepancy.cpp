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

#include <pbrt/util/lowdiscrepancy.h>

#include <pbrt/util/math.h>
#include <pbrt/util/primes.h>
#include <pbrt/util/print.h>
#include <pbrt/util/stats.h>

namespace pbrtv4 {

std::string DigitPermutation::ToString() const {
    std::string s = StringPrintf(
        "[ DigitPermitation base: %d nDigits: %d permutations: ", base, nDigits);
    for (int digitIndex = 0; digitIndex < nDigits; ++digitIndex) {
        s += StringPrintf("[%d] ( ", digitIndex);
        for (int digitValue = 0; digitValue < base; ++digitValue) {
            s += StringPrintf("%d", permutations[digitIndex * base + digitValue]);
            if (digitValue != base - 1)
                s += ", ";
        }
        s += ") ";
    }

    return s + " ]";
}

std::string ToString(RandomizeStrategy r) {
    switch (r) {
    case RandomizeStrategy::None:
        return "None";
    case RandomizeStrategy::PermuteDigits:
        return "PermuteDigits";
    case RandomizeStrategy::FastOwen:
        return "FastOwen";
    case RandomizeStrategy::Owen:
        return "Owen";
    default:
        LOG_FATAL("Unhandled RandomizeStrategy");
        return "";
    }
}

// Low Discrepancy Function Definitions
pstd::vector<DigitPermutation> *ComputeRadicalInversePermutations(uint32_t seed,
                                                                  Allocator alloc) {
    pstd::vector<DigitPermutation> *perms =
        alloc.new_object<pstd::vector<DigitPermutation>>(alloc);
    perms->resize(PrimeTableSize);
    for (int i = 0; i < PrimeTableSize; ++i)
        (*perms)[i] = DigitPermutation(Primes[i], seed, alloc);
    return perms;
}

}  // namespace pbrtv4
