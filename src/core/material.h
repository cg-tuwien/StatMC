
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

#ifndef PBRT_CORE_MATERIAL_H
#define PBRT_CORE_MATERIAL_H

// core/material.h*
#include "pbrt.h"
#include "memory.h"
#include "spectrum.h"

namespace pbrt {

// TransportMode Declarations
enum class TransportMode { Radiance, Importance };

// Material Declarations
class Material {
  public:
    // Material Interface
    virtual void ComputeScatteringFunctions(SurfaceInteraction *si,
                                            MemoryArena &arena,
                                            TransportMode mode,
                                            bool allowMultipleLobes) const = 0;
    Material(const unsigned long long id = 0) : id(id) {};
    virtual ~Material();
    virtual unsigned long long GetId() const;
    virtual RGBSpectrum GetAlbedo(SurfaceInteraction *si) const;
    static void Bump(const std::shared_ptr<Texture<Float>> &d,
                     SurfaceInteraction *si);

  protected:
    // Material Protected Data
    Float         *albedoLUT;
    unsigned char  albedoLUTNDims;
    unsigned char *albedoLUTMaxIndices;
    unsigned int  *albedoLUTOffsets;
    unsigned int  *albedoLUTRGBOffsets;

    // Material Protected Methods
    // These allocation methods intentionally do not access the members
    // directly for the possibility of using these for other LUTs in future.
    void AllocateLUT(
              Float          *source,
        const unsigned char   sourceNDims,
              unsigned char  *sourceMaxIndices,
              unsigned int   *sourceOffsets,
              Float         *&target, // https://stackoverflow.com/a/37013270
              unsigned char  &targetNDims,
              unsigned char *&targetMaxIndices,
              unsigned int  *&targetOffsets,
              unsigned int  *&targetRGBOffsets
    ) const;
    void DeallocateLUT(
        const Float          *source,
        const unsigned char  *sourceMaxIndices,
        const unsigned int   *sourceOffsets,
              Float         *&target,
              unsigned char *&targetMaxIndices,
              unsigned int  *&targetOffsets,
              unsigned int  *&targetRGBOffsets
    ) const;

  private:
    // Material Private Data
    unsigned long long id;

    // Material Macros for Private Methods
#define LUT_SET_REDUCIBILITY(i) \
    reducible = true; \
    reducibilities[i] = true; \
    nDims--;
#define LUT_SET_INDICES(i, val) \
    indices[0][i] = val; \
    indices[1][i] = val; \
    indices[2][i] = val;
#define LUT_SET_INDICES_SPECTRUM(i, spectrum) \
    Float rgb[3]; \
    spectrum.ToRGB(rgb); \
    indices[0][i] = rgb[0]; \
    indices[1][i] = rgb[1]; \
    indices[2][i] = rgb[2];

    // Material Private Methods
    virtual void GetLUTReducibilities(
        bool &reducible,
        bool *reducibilities,
        unsigned char &nDims
    ) const {
        LOG(FATAL) << "Material::GetLUTReducibilities() is not implemented!";
    };
    virtual void GetLUTReductionIndices(
        std::vector<std::vector<Float>> &indices
    ) const {
        LOG(FATAL) << "Material::GetLUTReductionIndices() is not implemented!";
    };
    virtual void GetLUTIndices(
        SurfaceInteraction *si,
        std::vector<std::vector<Float>> &indices
    ) const {
        LOG(FATAL) << "Material::GetLUTIndices() is not implemented!";
    };
};

}  // namespace pbrt

#endif  // PBRT_CORE_MATERIAL_H
