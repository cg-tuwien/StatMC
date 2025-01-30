
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


// materials/matte.cpp*
#include "materials/matte.h"
#include "paramset.h"
#include "reflection.h"
#include "interaction.h"
#include "texture.h"
#include "spectrum.h"
#include "statistics/luts/mattealbedo.h"

namespace pbrt {

// MatteMaterial Method Definitions
MatteMaterial::MatteMaterial(
    const std::shared_ptr<Texture<Spectrum>> &Kd,
    const std::shared_ptr<Texture<Float>> &sigma,
    const std::shared_ptr<Texture<Float>> &bumpMap,
    const unsigned long long id
) : Material(id),
    Kd(Kd),
    sigma(sigma),
    bumpMap(bumpMap)
{
    AllocateLUT(
        &matteAlbedoLUT[0],
         matteAlbedoLUTNDims,
        &matteAlbedoLUTMaxIndices[0],
        &matteAlbedoLUTOffsets[0],
         albedoLUT,           // Potentially allocated with new (we might just point to the data given above)
         albedoLUTNDims,
         albedoLUTMaxIndices, // Potentially allocated with new (we might just point to the data given above)
         albedoLUTOffsets,    // Potentially allocated with new (we might just point to the data given above)
         albedoLUTRGBOffsets  // Allocated with new
    );
}

MatteMaterial::~MatteMaterial() {
    DeallocateLUT(
        &matteAlbedoLUT[0],
        &matteAlbedoLUTMaxIndices[0],
        &matteAlbedoLUTOffsets[0],
         albedoLUT,
         albedoLUTMaxIndices,
         albedoLUTOffsets,
         albedoLUTRGBOffsets
    );
}

void MatteMaterial::ComputeScatteringFunctions(SurfaceInteraction *si,
                                               MemoryArena &arena,
                                               TransportMode mode,
                                               bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);

    // Evaluate textures for _MatteMaterial_ material and allocate BRDF
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);
    Spectrum r = Kd->Evaluate(*si).Clamp();
    Float sig = Clamp(sigma->Evaluate(*si), 0, 90);
    if (!r.IsBlack()) {
        if (sig == 0)
            si->bsdf->Add(ARENA_ALLOC(arena, LambertianReflection)(r));
        else
            si->bsdf->Add(ARENA_ALLOC(arena, OrenNayar)(r, sig));
    }
}

RGBSpectrum MatteMaterial::GetAlbedo(SurfaceInteraction *si) const {
    RGBSpectrum r = Kd->Evaluate(*si).Clamp();

    return r * Material::GetAlbedo(si);
}

void MatteMaterial::GetLUTReducibilities(
    bool &reducible,
    bool *reducibilities,
    unsigned char &nDims
) const {
    if (sigma->IsConstant()) {
        LUT_SET_REDUCIBILITY(1)
    }
}

void MatteMaterial::GetLUTReductionIndices(
    std::vector<std::vector<Float>> &indices
) const {
    if (sigma->IsConstant()) {
        const Float sigmaMapped = Clamp(InverseLerp(sigma->Evaluate(), 0.f, 90.f), 0.f, 1.f);
        LUT_SET_INDICES(1, sigmaMapped)
    }
}

void MatteMaterial::GetLUTIndices(
    SurfaceInteraction *si,
    std::vector<std::vector<Float>> &indices
) const {
    const Float cosThetaMapped = Clamp(InverseLerp(Dot(si->wo, si->shading.n), CosEpsilon, 1.f), 0.f, 1.f); // Transform wo.z to local space
    LUT_SET_INDICES(0, cosThetaMapped)

    if (!sigma->IsConstant()) {
        const Float sigmaMapped = Clamp(InverseLerp(sigma->Evaluate(*si), 0.f, 90.f), 0.f, 1.f);
        LUT_SET_INDICES(1, sigmaMapped)
    }
}

MatteMaterial *CreateMatteMaterial(const TextureParams &mp,
                                   const unsigned long long id) {
    std::shared_ptr<Texture<Spectrum>> Kd =
        mp.GetSpectrumTexture("Kd", Spectrum(0.5f));
    std::shared_ptr<Texture<Float>> sigma = mp.GetFloatTexture("sigma", 0.f);
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    return new MatteMaterial(Kd, sigma, bumpMap, id);
}

}  // namespace pbrt
