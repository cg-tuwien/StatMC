
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


// materials/glass.cpp*
#include "materials/glass.h"
#include "spectrum.h"
#include "reflection.h"
#include "paramset.h"
#include "texture.h"
#include "interaction.h"
#include "statistics/luts/glassalbedo.h"

namespace pbrt {

// GlassMaterial Method Definitions
GlassMaterial::GlassMaterial(
    const std::shared_ptr<Texture<Spectrum>> &Kr,
    const std::shared_ptr<Texture<Spectrum>> &Kt,
    const std::shared_ptr<Texture<Float>> &uRoughness,
    const std::shared_ptr<Texture<Float>> &vRoughness,
    const std::shared_ptr<Texture<Float>> &index,
    const std::shared_ptr<Texture<Float>> &bumpMap,
    bool remapRoughness,
    const unsigned long long id
) : Material(id),
    Kr(Kr),
    Kt(Kt),
    uRoughness(uRoughness),
    vRoughness(vRoughness),
    index(index),
    bumpMap(bumpMap),
    remapRoughness(remapRoughness)
{
    AllocateLUT(
        &glassAlbedoLUT[0],
         glassAlbedoLUTNDims,
        &glassAlbedoLUTMaxIndices[0],
        &glassAlbedoLUTOffsets[0],
         albedoLUT,           // Potentially allocated with new (we might just point to the data given above)
         albedoLUTNDims,
         albedoLUTMaxIndices, // Potentially allocated with new (we might just point to the data given above)
         albedoLUTOffsets,    // Potentially allocated with new (we might just point to the data given above)
         albedoLUTRGBOffsets  // Allocated with new
    );
}

GlassMaterial::~GlassMaterial() {
    DeallocateLUT(
        &glassAlbedoLUT[0],
        &glassAlbedoLUTMaxIndices[0],
        &glassAlbedoLUTOffsets[0],
         albedoLUT,
         albedoLUTMaxIndices,
         albedoLUTOffsets,
         albedoLUTRGBOffsets
    );
}

void GlassMaterial::ComputeScatteringFunctions(SurfaceInteraction *si,
                                               MemoryArena &arena,
                                               TransportMode mode,
                                               bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    Float eta = index->Evaluate(*si);
    Float urough = uRoughness->Evaluate(*si);
    Float vrough = vRoughness->Evaluate(*si);
    Spectrum R = Kr->Evaluate(*si).Clamp();
    Spectrum T = Kt->Evaluate(*si).Clamp();
    // Initialize _bsdf_ for smooth or rough dielectric
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si, eta);

    if (R.IsBlack() && T.IsBlack()) return;

    bool isSpecular = urough == 0 && vrough == 0;
    if (isSpecular && allowMultipleLobes) {
        si->bsdf->Add(
            ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, eta, mode));
    } else {
        if (remapRoughness) {
            urough = TrowbridgeReitzDistribution::RoughnessToAlpha(urough);
            vrough = TrowbridgeReitzDistribution::RoughnessToAlpha(vrough);
        }
        MicrofacetDistribution *distrib =
            isSpecular ? nullptr
                       : ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(
                             urough, vrough);
        if (!R.IsBlack()) {
            Fresnel *fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, eta);
            if (isSpecular)
                si->bsdf->Add(
                    ARENA_ALLOC(arena, SpecularReflection)(R, fresnel));
            else
                si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(
                    R, distrib, fresnel));
        }
        if (!T.IsBlack()) {
            if (isSpecular)
                si->bsdf->Add(ARENA_ALLOC(arena, SpecularTransmission)(
                    T, 1.f, eta, mode));
            else
                si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetTransmission)(
                    T, distrib, 1.f, eta, mode));
        }
    }
}

void GlassMaterial::GetLUTReducibilities(
    bool &reducible,
    bool *reducibilities,
    unsigned char &nDims
) const {
    if (Kr->IsConstant()) {
        LUT_SET_REDUCIBILITY(1)
    }
    if (Kt->IsConstant()) {
        LUT_SET_REDUCIBILITY(2)
    }
    if (uRoughness->IsConstant()) {
        LUT_SET_REDUCIBILITY(3)
    }
    if (vRoughness->IsConstant()) {
        LUT_SET_REDUCIBILITY(4)
    }
    if (index->IsConstant()) {
        LUT_SET_REDUCIBILITY(5)
    }
}

// These two methods look redundant, but there are key differences.
// The first one returns indices for all source LUT dimensions for parameters that are constant.
// The second one return indices for parameters that are NOT constant.
// Moreover, this one does not require a surface interaction, whereas the other one does.
// Lastly, the index calculation is handled differently (source vs. target LUT indices).
void GlassMaterial::GetLUTReductionIndices(
    std::vector<std::vector<Float>> &indices
) const {
    if (Kr->IsConstant()) {
        const RGBSpectrum krMapped = Kr->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(1, krMapped)
    }
    if (Kt->IsConstant()) {
        const RGBSpectrum ktMapped = Kt->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(2, ktMapped)
    }
    if (uRoughness->IsConstant()) {
        Float uRough = uRoughness->Evaluate();
        if (remapRoughness)
            uRough = TrowbridgeReitzDistribution::RoughnessToAlpha(uRough);
        const Float uRoughnessMapped = Clamp(InverseLerp(uRough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(3, uRoughnessMapped)
    }
    if (vRoughness->IsConstant()) {
        Float vRough = vRoughness->Evaluate();
        if (remapRoughness)
            vRough = TrowbridgeReitzDistribution::RoughnessToAlpha(vRough);
        const Float vRoughnessMapped = Clamp(InverseLerp(vRough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(4, vRoughnessMapped)
    }
    if (index->IsConstant()) {
        const Float indexMapped = Clamp(InverseLerp(index->Evaluate(), 1.f + Epsilon, 2.42f), 0.f, 1.f);
        LUT_SET_INDICES(5, indexMapped)
    }
}

void GlassMaterial::GetLUTIndices(
    SurfaceInteraction *si,
    std::vector<std::vector<Float>> &indices
) const {
    const Float cosThetaMapped = Clamp(InverseLerp(Dot(si->wo, si->shading.n), CosEpsilon, 1.f), 0.f, 1.f); // Transform wo.z to local space
    LUT_SET_INDICES(0, cosThetaMapped)

    unsigned char i = 1;
    if (!Kr->IsConstant()) {
        const RGBSpectrum krMapped = Kr->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(i, krMapped)
        i++;
    }
    if (!Kt->IsConstant()) {
        const RGBSpectrum ktMapped = Kt->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(i, ktMapped)
        i++;
    }
    if (!uRoughness->IsConstant()) {
        Float uRough = uRoughness->Evaluate(*si);
        if (remapRoughness)
            uRough = TrowbridgeReitzDistribution::RoughnessToAlpha(uRough);
        const Float uRoughnessMapped = Clamp(InverseLerp(uRough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, uRoughnessMapped)
        i++;
    }
    if (!vRoughness->IsConstant()) {
        Float vRough = vRoughness->Evaluate(*si);
        if (remapRoughness)
            vRough = TrowbridgeReitzDistribution::RoughnessToAlpha(vRough);
        const Float vRoughnessMapped = Clamp(InverseLerp(vRough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, vRoughnessMapped)
        i++;
    }
    if (!index->IsConstant()) {
        const Float indexMapped = Clamp(InverseLerp(index->Evaluate(*si), 1.f + Epsilon, 2.42f), 0.f, 1.f);
        LUT_SET_INDICES(i, indexMapped)
    }
}

GlassMaterial *CreateGlassMaterial(const TextureParams &mp,
                                   const unsigned long long id) {
    std::shared_ptr<Texture<Spectrum>> Kr =
        mp.GetSpectrumTexture("Kr", Spectrum(1.f));
    std::shared_ptr<Texture<Spectrum>> Kt =
        mp.GetSpectrumTexture("Kt", Spectrum(1.f));
    std::shared_ptr<Texture<Float>> eta = mp.GetFloatTextureOrNull("eta");
    if (!eta) eta = mp.GetFloatTexture("index", 1.5f);
    std::shared_ptr<Texture<Float>> roughu =
        mp.GetFloatTexture("uroughness", 0.f);
    std::shared_ptr<Texture<Float>> roughv =
        mp.GetFloatTexture("vroughness", 0.f);
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    bool remapRoughness = mp.FindBool("remaproughness", true);
    return new GlassMaterial(Kr, Kt, roughu, roughv, eta, bumpMap,
                             remapRoughness, id);
}

}  // namespace pbrt
