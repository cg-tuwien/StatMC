
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


// materials/plastic.cpp*
#include "materials/plastic.h"
#include "spectrum.h"
#include "reflection.h"
#include "paramset.h"
#include "texture.h"
#include "interaction.h"
#include "statistics/luts/plasticalbedo.h"

namespace pbrt {

// PlasticMaterial Method Definitions
PlasticMaterial::PlasticMaterial(
    const std::shared_ptr<Texture<Spectrum>> &Kd,
    const std::shared_ptr<Texture<Spectrum>> &Ks,
    const std::shared_ptr<Texture<Float>> &roughness,
    const std::shared_ptr<Texture<Float>> &bumpMap,
    bool remapRoughness,
    const unsigned long long id
) : Material(id),
    Kd(Kd),
    Ks(Ks),
    roughness(roughness),
    bumpMap(bumpMap),
    remapRoughness(remapRoughness)
{
    AllocateLUT(
        &plasticAlbedoLUT[0],
         plasticAlbedoLUTNDims,
        &plasticAlbedoLUTMaxIndices[0],
        &plasticAlbedoLUTOffsets[0],
         albedoLUT,           // Potentially allocated with new (we might just point to the data given above)
         albedoLUTNDims,
         albedoLUTMaxIndices, // Potentially allocated with new (we might just point to the data given above)
         albedoLUTOffsets,    // Potentially allocated with new (we might just point to the data given above)
         albedoLUTRGBOffsets  // Allocated with new
    );
}

PlasticMaterial::~PlasticMaterial() {
    DeallocateLUT(
        &plasticAlbedoLUT[0],
        &plasticAlbedoLUTMaxIndices[0],
        &plasticAlbedoLUTOffsets[0],
         albedoLUT,
         albedoLUTMaxIndices,
         albedoLUTOffsets,
         albedoLUTRGBOffsets
    );
}

void PlasticMaterial::ComputeScatteringFunctions(
    SurfaceInteraction *si, MemoryArena &arena, TransportMode mode,
    bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);
    // Initialize diffuse component of plastic material
    Spectrum kd = Kd->Evaluate(*si).Clamp();
    if (!kd.IsBlack())
        si->bsdf->Add(ARENA_ALLOC(arena, LambertianReflection)(kd));

    // Initialize specular component of plastic material
    Spectrum ks = Ks->Evaluate(*si).Clamp();
    if (!ks.IsBlack()) {
        Fresnel *fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.5f, 1.f);
        // Create microfacet distribution _distrib_ for plastic material
        Float rough = roughness->Evaluate(*si);
        if (remapRoughness)
            rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
        MicrofacetDistribution *distrib =
            ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(rough, rough);
        BxDF *spec =
            ARENA_ALLOC(arena, MicrofacetReflection)(ks, distrib, fresnel);
        si->bsdf->Add(spec);
    }
}

void PlasticMaterial::GetLUTReducibilities(
    bool &reducible,
    bool *reducibilities,
    unsigned char &nDims
) const {
    if (Kd->IsConstant()) {
        LUT_SET_REDUCIBILITY(1)
    }
    if (Ks->IsConstant()) {
        LUT_SET_REDUCIBILITY(2)
    }
    if (roughness->IsConstant()) {
        LUT_SET_REDUCIBILITY(3)
    }
}

void PlasticMaterial::GetLUTReductionIndices(
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
    if (roughness->IsConstant()) {
        Float rough = roughness->Evaluate();
        if (remapRoughness)
            rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
        const Float roughnessMapped = Clamp(InverseLerp(rough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(3, roughnessMapped)
    }
}

void PlasticMaterial::GetLUTIndices(
    SurfaceInteraction *si,
    std::vector<std::vector<Float>> &indices
) const {
    const Float cosThetaMapped = Clamp(InverseLerp(Dot(si->wo, si->shading.n), CosEpsilon, 1.f), 0.f, 1.f); // Transform wo.z to local space
    LUT_SET_INDICES(0, cosThetaMapped)

    unsigned char i = 1;
    if (!Kd->IsConstant()) {
        const RGBSpectrum kdMapped = Kd->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(i, kdMapped)
        i++;
    }
    if (!Ks->IsConstant()) {
        const RGBSpectrum ksMapped = Ks->Evaluate(*si).Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(i, ksMapped)
        i++;
    }
    if (!roughness->IsConstant()) {
        Float rough = roughness->Evaluate(*si);
        if (remapRoughness)
            rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
        const Float roughnessMapped = Clamp(InverseLerp(rough, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, roughnessMapped)
    }
}

PlasticMaterial *CreatePlasticMaterial(const TextureParams &mp,
                                       const unsigned long long id) {
    std::shared_ptr<Texture<Spectrum>> Kd =
        mp.GetSpectrumTexture("Kd", Spectrum(0.25f));
    std::shared_ptr<Texture<Spectrum>> Ks =
        mp.GetSpectrumTexture("Ks", Spectrum(0.25f));
    std::shared_ptr<Texture<Float>> roughness =
        mp.GetFloatTexture("roughness", .1f);
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    bool remapRoughness = mp.FindBool("remaproughness", true);
    return new PlasticMaterial(Kd, Ks, roughness, bumpMap, remapRoughness, id);
}

}  // namespace pbrt
