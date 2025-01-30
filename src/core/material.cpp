
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


// core/material.cpp*
#include "material.h"
#include "primitive.h"
#include "texture.h"
#include "stats.h"
#include "spectrum.h"
#include "reflection.h"
#include "statistics/lut.h"
#include <bitset>

namespace pbrt {

// Material Method Definitions
Material::~Material() {}

unsigned long long Material::GetId() const {
    return id;
}

RGBSpectrum Material::GetAlbedo(SurfaceInteraction *si) const {
    std::vector<std::vector<Float>> indices(3, std::vector<Float>(albedoLUTNDims));

    GetLUTIndices(si, indices);

    const Float albedoRGB[3] = {
        LookupTable(
            &albedoLUT[albedoLUTRGBOffsets[0]],
             albedoLUTNDims,
             albedoLUTMaxIndices,
             albedoLUTOffsets,
            &indices[0][0]
        ),
        LookupTable(
            &albedoLUT[albedoLUTRGBOffsets[1]],
             albedoLUTNDims,
             albedoLUTMaxIndices,
             albedoLUTOffsets,
            &indices[1][0]
        ),
        LookupTable(
            &albedoLUT[albedoLUTRGBOffsets[2]],
             albedoLUTNDims,
             albedoLUTMaxIndices,
             albedoLUTOffsets,
            &indices[2][0]
        )
    };

    return RGBSpectrum::FromRGB(albedoRGB);
}

void Material::Bump(const std::shared_ptr<Texture<Float>> &d,
                    SurfaceInteraction *si) {
    // Compute offset positions and evaluate displacement texture
    SurfaceInteraction siEval = *si;

    // Shift _siEval_ _du_ in the $u$ direction
    Float du = .5f * (std::abs(si->dudx) + std::abs(si->dudy));
    // The most common reason for du to be zero is for ray that start from
    // light sources, where no differentials are available. In this case,
    // we try to choose a small enough du so that we still get a decently
    // accurate bump value.
    if (du == 0) du = .0005f;
    siEval.p = si->p + du * si->shading.dpdu;
    siEval.uv = si->uv + Vector2f(du, 0.f);
    siEval.n = Normalize((Normal3f)Cross(si->shading.dpdu, si->shading.dpdv) +
                         du * si->dndu);
    Float uDisplace = d->Evaluate(siEval);

    // Shift _siEval_ _dv_ in the $v$ direction
    Float dv = .5f * (std::abs(si->dvdx) + std::abs(si->dvdy));
    if (dv == 0) dv = .0005f;
    siEval.p = si->p + dv * si->shading.dpdv;
    siEval.uv = si->uv + Vector2f(0.f, dv);
    siEval.n = Normalize((Normal3f)Cross(si->shading.dpdu, si->shading.dpdv) +
                         dv * si->dndv);
    Float vDisplace = d->Evaluate(siEval);
    Float displace = d->Evaluate(*si);

    // Compute bump-mapped differential geometry
    Vector3f dpdu = si->shading.dpdu +
                    (uDisplace - displace) / du * Vector3f(si->shading.n) +
                    displace * Vector3f(si->shading.dndu);
    Vector3f dpdv = si->shading.dpdv +
                    (vDisplace - displace) / dv * Vector3f(si->shading.n) +
                    displace * Vector3f(si->shading.dndv);
    si->SetShadingGeometry(dpdu, dpdv, si->shading.dndu, si->shading.dndv,
                           false);
}

void Material::AllocateLUT(
          Float          *source,
    const unsigned char   sourceNDims,
          unsigned char  *sourceMaxIndices,
          unsigned int   *sourceOffsets,
          Float         *&target,
          unsigned char  &targetNDims,
          unsigned char *&targetMaxIndices,
          unsigned int  *&targetOffsets,
          unsigned int  *&targetRGBOffsets
) const {
    ProfilePhase p(Prof::ReduceLUT);

    // Initialize variables
    targetNDims = sourceNDims;

    // Identify reducibilities
    bool reducible = false;
    bool reducibilities[sourceNDims];
    for (unsigned char i = 0; i < sourceNDims; i++)
        reducibilities[i] = false;
    GetLUTReducibilities(reducible, &reducibilities[0], targetNDims);

    if (reducible) {
        // Calculate target LUT parameters
        targetMaxIndices = new unsigned char[targetNDims];
        unsigned char targetIndices[targetNDims];
        unsigned char targetLengths[targetNDims];
        unsigned char offset = 0;
        for (unsigned char i = 0; i < targetNDims; i++) {
            if (reducibilities[i])
                offset++;
            targetIndices[i] = 0;
            targetMaxIndices[i] = sourceMaxIndices[i + offset];
            targetLengths[i] = targetMaxIndices[i] + 1;
        }

        // Allocate memory for target LUT
        unsigned int length = 1;
        for (unsigned char i = 0; i < targetNDims; i++)
            length *= targetLengths[i];
        target = new Float[3 * length]; // number of spectrum coefficients

        unsigned char targetDimIndex = 0;
        std::vector<std::vector<Float>> sourceIndices(3, std::vector<Float>(sourceNDims));
        while (true) {
            // Assign source LUT value to target LUT value
            offset = 0;
            Float sourceIndex;
            for (unsigned char i = 0; i < sourceNDims; i++) {
                if (!reducibilities[i]) {
                    sourceIndex = (Float)targetIndices[i - offset] / targetMaxIndices[i - offset];
                    sourceIndices[0][i] = sourceIndex;
                    sourceIndices[1][i] = sourceIndex;
                    sourceIndices[2][i] = sourceIndex;
                } else
                    offset++;
            }
            GetLUTReductionIndices(sourceIndices);

            // Reducing spectrum parameters requires storing each spectrum coefficient individually
            unsigned int targetIndex = 0;
            for (unsigned char c = 0; c < 3; c++) {
                targetIndex = c;
                for (char i = targetNDims - 1; i > -1; i--) {
                    targetIndex *= targetLengths[i];
                    targetIndex += targetIndices[i];
                }
                target[targetIndex] = LookupTable(
                     source,
                     sourceNDims,
                     sourceMaxIndices,
                     sourceOffsets,
                    &sourceIndices[c][0]
                );
            }

            // Increment indices
            targetIndices[0]++;

            while (targetIndices[targetDimIndex] == targetLengths[targetDimIndex]) {
                if (targetDimIndex == targetNDims - 1)
                    goto breakAll;
                targetIndices[targetDimIndex++] = 0;
                targetIndices[targetDimIndex]++;
            }

            targetDimIndex = 0;
        }

        breakAll:

        // Calculate offsets
        const unsigned short targetOffsetCount = 1 << targetNDims;
        targetOffsets = new unsigned int[targetOffsetCount];
        for (unsigned short i = 0; i < targetOffsetCount; i++) {
            targetOffsets[i] = 0;
            std::bitset<6> bits(i);
            unsigned int increment = 1;
            for (unsigned char j = 0; j < targetNDims; j++) {
                if (bits[j])
                    targetOffsets[i] += increment;
                increment *= targetLengths[j];
            }
        }
 
        targetRGBOffsets = new unsigned int[3];
        for (unsigned char c = 0; c < 3; c++)
            targetRGBOffsets[c] = c * length;
    } else {
        // LUT is not reducible: copy
        target           = source;
        targetNDims      = sourceNDims;
        targetMaxIndices = sourceMaxIndices;
        targetOffsets    = sourceOffsets;

        targetRGBOffsets = new unsigned int[3];
        for (unsigned char c = 0; c < 3; c++)
            targetRGBOffsets[c] = 0;
    }
}

void Material::DeallocateLUT(
    const Float          *source,
    const unsigned char  *sourceMaxIndices,
    const unsigned int   *sourceOffsets,
          Float         *&target,
          unsigned char *&targetMaxIndices,
          unsigned int  *&targetOffsets,
          unsigned int  *&targetRGBOffsets
) const {
    if (target != source)
        delete[] target;
    if (targetMaxIndices != sourceMaxIndices)
        delete[] targetMaxIndices;
    if (targetOffsets != sourceOffsets)
        delete[] targetOffsets;
    delete[] targetRGBOffsets;
}

}  // namespace pbrt
