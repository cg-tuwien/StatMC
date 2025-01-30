
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


// materials/substrate.cpp*
#include "materials/substrate.h"
#include "spectrum.h"
#include "reflection.h"
#include "paramset.h"
#include "texture.h"
#include "interaction.h"
#include "statistics/luts/substratealbedo.h"

namespace pbrt {

// SubstrateMaterial Method Definitions
SubstrateMaterial::SubstrateMaterial(
    const std::shared_ptr<Texture<Spectrum>> &Kd,
    const std::shared_ptr<Texture<Spectrum>> &Ks,
    const std::shared_ptr<Texture<Float>> &nu,
    const std::shared_ptr<Texture<Float>> &nv,
    const std::shared_ptr<Texture<Float>> &bumpMap,
    bool remapRoughness,
    const unsigned long long id
) : Material(id),
    Kd(Kd),
    Ks(Ks),
    nu(nu),
    nv(nv),
    bumpMap(bumpMap),
    remapRoughness(remapRoughness)
{
    AllocateLUT(
        &substrateAlbedoLUT[0],
         substrateAlbedoLUTNDims,
        &substrateAlbedoLUTMaxIndices[0],
        &substrateAlbedoLUTOffsets[0],
         albedoLUT,           // Potentially allocated with new (we might just point to the data given above)
         albedoLUTNDims,
         albedoLUTMaxIndices, // Potentially allocated with new (we might just point to the data given above)
         albedoLUTOffsets,    // Potentially allocated with new (we might just point to the data given above)
         albedoLUTRGBOffsets  // Allocated with new
    );
}

SubstrateMaterial::~SubstrateMaterial() {
    DeallocateLUT(
        &substrateAlbedoLUT[0],
        &substrateAlbedoLUTMaxIndices[0],
        &substrateAlbedoLUTOffsets[0],
         albedoLUT,
         albedoLUTMaxIndices,
         albedoLUTOffsets,
         albedoLUTRGBOffsets
    );
}

void SubstrateMaterial::ComputeScatteringFunctions(
    SurfaceInteraction *si, MemoryArena &arena, TransportMode mode,
    bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);
    Spectrum d = Kd->Evaluate(*si).Clamp();
    Spectrum s = Ks->Evaluate(*si).Clamp();
    Float roughu = nu->Evaluate(*si);
    Float roughv = nv->Evaluate(*si);

    if (!d.IsBlack() || !s.IsBlack()) {
        if (remapRoughness) {
            roughu = TrowbridgeReitzDistribution::RoughnessToAlpha(roughu);
            roughv = TrowbridgeReitzDistribution::RoughnessToAlpha(roughv);
        }
        MicrofacetDistribution *distrib =
            ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(roughu, roughv);
        si->bsdf->Add(ARENA_ALLOC(arena, FresnelBlend)(d, s, distrib));
    }
}

void SubstrateMaterial::GetLUTReducibilities(bool          &reducible,
                                             bool          *reducibilities,
                                             unsigned char &nDims) const {
    if (Kd->IsConstant()) {
        LUT_SET_REDUCIBILITY(1)
    }
    if (Ks->IsConstant()) {
        LUT_SET_REDUCIBILITY(2)
    }
    if (nu->IsConstant()) {
        LUT_SET_REDUCIBILITY(3)
    }
    if (nv->IsConstant()) {
        LUT_SET_REDUCIBILITY(4)
    }
}

void SubstrateMaterial::GetLUTReductionIndices(
    std::vector<std::vector<Float>> &indices
) const {
    if (Kd->IsConstant()) {
        const RGBSpectrum kdMapped = Kd->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(1, kdMapped)
    }
    if (Ks->IsConstant()) {
        const RGBSpectrum ksMapped = Ks->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(2, ksMapped)
    }
    if (nu->IsConstant()) {
        Float roughU = nu->Evaluate();
        if (remapRoughness)
            roughU = TrowbridgeReitzDistribution::RoughnessToAlpha(roughU);
        const Float nuMapped = Clamp(InverseLerp(roughU, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(3, nuMapped)
    }
    if (nv->IsConstant()) {
        Float roughV = nv->Evaluate();
        if (remapRoughness)
            roughV = TrowbridgeReitzDistribution::RoughnessToAlpha(roughV);
        const Float nvMapped = Clamp(InverseLerp(roughV, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(4, nvMapped)
    }
}

void SubstrateMaterial::GetLUTIndices(
    SurfaceInteraction *si,
    std::vector<std::vector<Float>> &indices
) const {
    const Float cosThetaMapped = Clamp(InverseLerp(Dot(si->wo, si->shading.n), CosEpsilon, 1.f), 0.f, 1.f); // Transform wo.z to local space
    LUT_SET_INDICES(0, cosThetaMapped)

    unsigned char i = 1;
    if (!Kd->IsConstant()) {
        const RGBSpectrum kdMapped = Kd->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(1, kdMapped)
        i++;
    }
    if (!Ks->IsConstant()) {
        const RGBSpectrum ksMapped = Ks->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(2, ksMapped)
        i++;
    }
    if (!nu->IsConstant()) {
        Float roughU = nu->Evaluate(*si);
        if (remapRoughness)
            roughU = TrowbridgeReitzDistribution::RoughnessToAlpha(roughU);
        const Float nuMapped = Clamp(InverseLerp(roughU, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, nuMapped)
        i++;
    }
    if (!nv->IsConstant()) {
        Float roughV = nv->Evaluate(*si);
        if (remapRoughness)
            roughV = TrowbridgeReitzDistribution::RoughnessToAlpha(roughV);
        const Float nvMapped = Clamp(InverseLerp(roughV, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, nvMapped)
    }
}

SubstrateMaterial *CreateSubstrateMaterial(const TextureParams &mp,
                                           const unsigned long long id) {
    std::shared_ptr<Texture<Spectrum>> Kd =
        mp.GetSpectrumTexture("Kd", Spectrum(.5f));
    std::shared_ptr<Texture<Spectrum>> Ks =
        mp.GetSpectrumTexture("Ks", Spectrum(.5f));
    std::shared_ptr<Texture<Float>> uroughness =
        mp.GetFloatTexture("uroughness", .1f);
    std::shared_ptr<Texture<Float>> vroughness =
        mp.GetFloatTexture("vroughness", .1f);
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    bool remapRoughness = mp.FindBool("remaproughness", true);
    return new SubstrateMaterial(Kd, Ks, uroughness, vroughness, bumpMap,
                                 remapRoughness, id);
}

}  // namespace pbrt
