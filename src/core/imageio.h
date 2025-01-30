
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/*
    This file contains modifications to the original pbrt source code for the
    paper "A Statistical Approach to Monte Carlo Denoising"
    (https://www.cg.tuwien.ac.at/StatMC).
    
    Copyright © 2024-2025 Hiroyuki Sakai for the modifications.
    Original copyright and license (refer to the top of the file) remain
    unaffected.
 */

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_CORE_IMAGEIO_H
#define PBRT_CORE_IMAGEIO_H

// core/imageio.h*
#include "pbrt.h"
#include "geometry.h"
#include <cctype>

namespace pbrt {

// ImageIO Declarations
std::unique_ptr<RGBSpectrum[]> ReadImage(const std::string &name,
                                         Point2i *resolution);
RGBSpectrum *ReadImageEXR(const std::string &name, int *width,
                          int *height, Bounds2i *dataWindow = nullptr,
                          Bounds2i *displayWindow = nullptr);

void WriteImage(const std::string &name, const Float *rgb,
                const Bounds2i &outputBounds, const Point2i &totalResolution);

template<typename T>
bool WriteImageBinary(const std::string &filename, const T *x,
                      const Bounds2i &outputBounds, int channelCount = 1) {
    Vector2i resolution = outputBounds.Diagonal();

    FILE *fp;

    fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        Error("Unable to open output binary file \"%s\"", filename.c_str());
        return false;
    }

    // Write the width and height; they must be positive.
    if (fprintf(fp, "%d %d %d\n", resolution.x, resolution.y, channelCount) < 0) goto fail;

    // Write the data from bottom left to upper right as specified by
    // http://netpbm.sourceforge.net/doc/pfm.html .
    // The raster is a sequence of pixels, packed one after another, with no
    // delimiters of any kind. They are grouped by row, with the pixels in each
    // row ordered left to right and the rows ordered bottom to top.
    for (int j = resolution.y - 1; j >= 0; j--) {
        const T *scanline = &x[j * resolution.x * channelCount];
        if (fwrite(scanline, sizeof(T), resolution.x * channelCount, fp) <
            (size_t)(resolution.x * channelCount))
            goto fail;
    }

    fclose(fp);
    return true;

fail:
    Error("Error writing binary file \"%s\"", filename.c_str());
    fclose(fp);
    return false;
}

}  // namespace pbrt

#endif  // PBRT_CORE_IMAGEIO_H
