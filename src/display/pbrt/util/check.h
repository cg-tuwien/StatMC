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

#ifndef PBRT_UTIL_CHECK_H
#define PBRT_UTIL_CHECK_H

#include <pbrt/pbrt.h>

#include <pbrt/util/log.h>
#include <pbrt/util/stats.h>

#include <functional>
#include <string>
#include <vector>

namespace pbrtv4 {

void PrintStackTrace();

#ifdef PBRT_IS_GPU_CODE

#define PBRT_V4_CHECK(x) assert(x)
#define PBRT_V4_CHECK_IMPL(a, b, op) assert((a)op(b))

#define PBRT_V4_CHECK_EQ(a, b) PBRT_V4_CHECK_IMPL(a, b, ==)
#define PBRT_V4_CHECK_NE(a, b) PBRT_V4_CHECK_IMPL(a, b, !=)
#define PBRT_V4_CHECK_GT(a, b) PBRT_V4_CHECK_IMPL(a, b, >)
#define PBRT_V4_CHECK_GE(a, b) PBRT_V4_CHECK_IMPL(a, b, >=)
#define PBRT_V4_CHECK_LT(a, b) PBRT_V4_CHECK_IMPL(a, b, <)
#define PBRT_V4_CHECK_LE(a, b) PBRT_V4_CHECK_IMPL(a, b, <=)

#else

// PBRT_V4_CHECK Macro Definitions
#define PBRT_V4_CHECK(x) (!(!(x) && (LOG_FATAL("Check failed: %s", #x), true)))

#define PBRT_V4_CHECK_EQ(a, b) PBRT_V4_CHECK_IMPL(a, b, ==)
#define PBRT_V4_CHECK_NE(a, b) PBRT_V4_CHECK_IMPL(a, b, !=)
#define PBRT_V4_CHECK_GT(a, b) PBRT_V4_CHECK_IMPL(a, b, >)
#define PBRT_V4_CHECK_GE(a, b) PBRT_V4_CHECK_IMPL(a, b, >=)
#define PBRT_V4_CHECK_LT(a, b) PBRT_V4_CHECK_IMPL(a, b, <)
#define PBRT_V4_CHECK_LE(a, b) PBRT_V4_CHECK_IMPL(a, b, <=)

// PBRT_V4_CHECK\_IMPL Macro Definition
#define PBRT_V4_CHECK_IMPL(a, b, op)                                                           \
    do {                                                                               \
        auto va = a;                                                                   \
        auto vb = b;                                                                   \
        if (!(va op vb))                                                               \
            LOG_FATAL("Check failed: %s " #op " %s with %s = %s, %s = %s", #a, #b, #a, \
                      va, #b, vb);                                                     \
    } while (false) /* swallow semicolon */

#endif  // PBRT_IS_GPU_CODE

#ifdef PBRT_DEBUG_BUILD

#define PBRT_V4_DCHECK(x) (CHECK(x))
#define PBRT_V4_DCHECK_EQ(a, b) PBRT_V4_CHECK_EQ(a, b)
#define PBRT_V4_DCHECK_NE(a, b) PBRT_V4_CHECK_NE(a, b)
#define PBRT_V4_DCHECK_GT(a, b) PBRT_V4_CHECK_GT(a, b)
#define PBRT_V4_DCHECK_GE(a, b) PBRT_V4_CHECK_GE(a, b)
#define PBRT_V4_DCHECK_LT(a, b) PBRT_V4_CHECK_LT(a, b)
#define PBRT_V4_DCHECK_LE(a, b) PBRT_V4_CHECK_LE(a, b)

#else

#define PBRT_V4_EMPTY_CHECK \
    do {            \
    } while (false) /* swallow semicolon */

// Use an empty check (rather than expanding the macros to nothing) to swallow the
// semicolon at the end, and avoid empty if-statements.
#define PBRT_V4_DCHECK(x) PBRT_V4_EMPTY_CHECK

#define PBRT_V4_DCHECK_EQ(a, b) PBRT_V4_EMPTY_CHECK
#define PBRT_V4_DCHECK_NE(a, b) PBRT_V4_EMPTY_CHECK
#define PBRT_V4_DCHECK_GT(a, b) PBRT_V4_EMPTY_CHECK
#define PBRT_V4_DCHECK_GE(a, b) PBRT_V4_EMPTY_CHECK
#define PBRT_V4_DCHECK_LT(a, b) PBRT_V4_EMPTY_CHECK
#define PBRT_V4_DCHECK_LE(a, b) PBRT_V4_EMPTY_CHECK

#endif

#define PBRT_V4_CHECK_RARE_TO_STRING(x) #x
#define PBRT_V4_CHECK_RARE_EXPAND_AND_TO_STRING(x) PBRT_V4_CHECK_RARE_TO_STRING(x)

#ifdef PBRT_IS_GPU_CODE

#define PBRT_V4_CHECK_RARE(freq, condition)
#define PBRT_V4_DCHECK_RARE(freq, condition)

#else

#define PBRT_V4_CHECK_RARE(freq, condition)                                                     \
    static_assert(std::is_floating_point<decltype(freq)>::value,                        \
                  "Expected floating-point frequency as first argument to PBRT_V4_CHECK_RARE"); \
    static_assert(std::is_integral<decltype(condition)>::value,                         \
                  "Expected Boolean condition as second argument to PBRT_V4_CHECK_RARE");       \
    do {                                                                                \
        static thread_local int64_t numTrue, total;                                     \
        static StatRegisterer reg([](StatsAccumulator &accum) {                         \
            accum.ReportRareCheck(__FILE__ " " PBRT_V4_CHECK_RARE_EXPAND_AND_TO_STRING(         \
                                      __LINE__) ": PBRT_V4_CHECK_RARE failed: " #condition,     \
                                  freq, numTrue, total);                                \
            numTrue = total = 0;                                                        \
        });                                                                             \
        ++total;                                                                        \
        if (condition)                                                                  \
            ++numTrue;                                                                  \
    } while (0)

#ifdef PBRT_DEBUG_BUILD
#define PBRT_V4_DCHECK_RARE(freq, condition) PBRT_V4_CHECK_RARE(freq, condition)
#else
#define PBRT_V4_DCHECK_RARE(freq, condition)
#endif  // NDEBUG

#endif  // PBRT_IS_GPU_CODE

// CheckCallbackScope Definition
class CheckCallbackScope {
  public:
    // CheckCallbackScope Public Methods
    CheckCallbackScope(std::function<std::string(void)> callback);

    ~CheckCallbackScope();

    CheckCallbackScope(const CheckCallbackScope &) = delete;
    CheckCallbackScope &operator=(const CheckCallbackScope &) = delete;

    static void Fail();

  private:
    // CheckCallbackScope Private Members
    static std::vector<std::function<std::string(void)>> callbacks;
};

}  // namespace pbrtv4

#endif  // PBRT_UTIL_CHECK_H
