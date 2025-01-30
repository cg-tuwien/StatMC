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

#ifndef PBRT_UTIL_COLORSPACE_H
#define PBRT_UTIL_COLORSPACE_H

#include <pbrt/pbrt.h>

#include <pbrt/util/color.h>
#include <pbrt/util/math.h>
#include <pbrt/util/spectrum.h>
#include <pbrt/util/vecmath.h>

#include <string>

namespace pbrtv4 {

// RGBColorSpace Definition
class RGBColorSpace {
  public:
    // RGBColorSpace Public Methods
    RGBColorSpace(Point2f r, Point2f g, Point2f b, Spectrum illuminant,
                  const RGBToSpectrumTable *rgbToSpectrumTable, Allocator alloc);

    PBRT_CPU_GPU
    RGBSigmoidPolynomial ToRGBCoeffs(RGB rgb) const;

    static void Init(Allocator alloc);

    // RGBColorSpace Public Members
    Point2f r, g, b, w;
    DenselySampledSpectrum illuminant;
    SquareMatrix<3> XYZFromRGB, RGBFromXYZ;
    static const RGBColorSpace *sRGB, *DCI_P3, *Rec2020, *ACES2065_1;

    PBRT_CPU_GPU
    bool operator==(const RGBColorSpace &cs) const {
        return (r == cs.r && g == cs.g && b == cs.b && w == cs.w &&
                rgbToSpectrumTable == cs.rgbToSpectrumTable);
    }
    PBRT_CPU_GPU
    bool operator!=(const RGBColorSpace &cs) const {
        return (r != cs.r || g != cs.g || b != cs.b || w != cs.w ||
                rgbToSpectrumTable != cs.rgbToSpectrumTable);
    }

    std::string ToString() const;

    PBRT_CPU_GPU
    RGB LuminanceVector() const {
        return RGB(XYZFromRGB[1][0], XYZFromRGB[1][1], XYZFromRGB[1][2]);
    }

    PBRT_CPU_GPU
    RGB ToRGB(XYZ xyz) const { return Mul<RGB>(RGBFromXYZ, xyz); }
    PBRT_CPU_GPU
    XYZ ToXYZ(RGB rgb) const { return Mul<XYZ>(XYZFromRGB, rgb); }

    static const RGBColorSpace *GetNamed(std::string name);
    static const RGBColorSpace *Lookup(Point2f r, Point2f g, Point2f b, Point2f w);

  private:
    // RGBColorSpace Private Members
    const RGBToSpectrumTable *rgbToSpectrumTable;
};

#ifdef PBRT_BUILD_GPU_RENDERER
extern PBRT_CONST RGBColorSpace *RGBColorSpace_sRGB;
extern PBRT_CONST RGBColorSpace *RGBColorSpace_DCI_P3;
extern PBRT_CONST RGBColorSpace *RGBColorSpace_Rec2020;
extern PBRT_CONST RGBColorSpace *RGBColorSpace_ACES2065_1;
#endif

SquareMatrix<3> ConvertRGBColorSpace(const RGBColorSpace &from, const RGBColorSpace &to);

}  // namespace pbrtv4

#endif  // PBRT_UTIL_COLORSPACE_H
