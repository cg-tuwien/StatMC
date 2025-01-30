
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

#ifndef PBRT_MATERIALS_TRANSLUCENT_H
#define PBRT_MATERIALS_TRANSLUCENT_H

// materials/translucent.h*
#include "pbrt.h"
#include "material.h"
#include "spectrum.h"

namespace pbrt {

// TranslucentMaterial Declarations
class TranslucentMaterial : public Material {
  public:
    // TranslucentMaterial Public Methods
    TranslucentMaterial(const std::shared_ptr<Texture<Spectrum>> &kd,
                        const std::shared_ptr<Texture<Spectrum>> &ks,
                        const std::shared_ptr<Texture<Float>> &rough,
                        const std::shared_ptr<Texture<Spectrum>> &refl,
                        const std::shared_ptr<Texture<Spectrum>> &trans,
                        const std::shared_ptr<Texture<Float>> &bump,
                        bool remap,
                        const unsigned long long id = 0);
    ~TranslucentMaterial();
    void ComputeScatteringFunctions(SurfaceInteraction *si, MemoryArena &arena,
                                    TransportMode mode,
                                    bool allowMultipleLobes) const;

  private:
    // TranslucentMaterial Private Data
    std::shared_ptr<Texture<Spectrum>> Kd, Ks;
    std::shared_ptr<Texture<Float>> roughness;
    std::shared_ptr<Texture<Spectrum>> reflect, transmit;
    std::shared_ptr<Texture<Float>> bumpMap;
    bool remapRoughness;

    // TranslucentMaterial Private Methods
    void GetLUTReducibilities(
        bool &reducible,
        bool *reducibilities,
        unsigned char &nDims
    ) const;
    void GetLUTReductionIndices(
        std::vector<std::vector<Float>> &indices
    ) const;
    void GetLUTIndices(
        SurfaceInteraction *si,
        std::vector<std::vector<Float>> &indices
    ) const;
};

TranslucentMaterial *CreateTranslucentMaterial(const TextureParams &mp,
                                               const unsigned long long id = 0);

}  // namespace pbrt

#endif  // PBRT_MATERIALS_TRANSLUCENT_H
