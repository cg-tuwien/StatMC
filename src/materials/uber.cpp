
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


// materials/uber.cpp*
#include "materials/uber.h"
#include "spectrum.h"
#include "reflection.h"
#include "texture.h"
#include "interaction.h"
#include "paramset.h"
#include "statistics/luts/uberalbedo.h"

namespace pbrt {

// UberMaterial Method Definitions
UberMaterial::UberMaterial(
    const std::shared_ptr<Texture<Spectrum>> &Kd,
    const std::shared_ptr<Texture<Spectrum>> &Ks,
    const std::shared_ptr<Texture<Spectrum>> &Kr,
    const std::shared_ptr<Texture<Spectrum>> &Kt,
    const std::shared_ptr<Texture<Float>> &roughness,
    const std::shared_ptr<Texture<Float>> &roughnessu,
    const std::shared_ptr<Texture<Float>> &roughnessv,
    const std::shared_ptr<Texture<Spectrum>> &opacity,
    const std::shared_ptr<Texture<Float>> &eta,
    const std::shared_ptr<Texture<Float>> &bumpMap,
    bool remapRoughness,
    const unsigned long long id
) : Material(id),
    Kd(Kd),
    Ks(Ks),
    Kr(Kr),
    Kt(Kt),
    opacity(opacity),
    roughness(roughness),
    roughnessu(roughnessu),
    roughnessv(roughnessv),
    eta(eta),
    bumpMap(bumpMap),
    remapRoughness(remapRoughness)
{
    AllocateLUT(
        &uberAlbedoLUT[0],
         uberAlbedoLUTNDims,
        &uberAlbedoLUTMaxIndices[0],
        &uberAlbedoLUTOffsets[0],
         albedoLUT,           // Potentially allocated with new (we might just point to the data given above)
         albedoLUTNDims,
         albedoLUTMaxIndices, // Potentially allocated with new (we might just point to the data given above)
         albedoLUTOffsets,    // Potentially allocated with new (we might just point to the data given above)
         albedoLUTRGBOffsets  // Allocated with new
    );
}

UberMaterial::~UberMaterial() {
    DeallocateLUT(
        &uberAlbedoLUT[0],
        &uberAlbedoLUTMaxIndices[0],
        &uberAlbedoLUTOffsets[0],
         albedoLUT,
         albedoLUTMaxIndices,
         albedoLUTOffsets,
         albedoLUTRGBOffsets
    );
}

void UberMaterial::ComputeScatteringFunctions(SurfaceInteraction *si,
                                              MemoryArena &arena,
                                              TransportMode mode,
                                              bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);
    Float e = eta->Evaluate(*si);

    Spectrum op = opacity->Evaluate(*si).Clamp();
    Spectrum t = (-op + Spectrum(1.f)).Clamp();
    if (!t.IsBlack()) {
        si->bsdf = ARENA_ALLOC(arena, BSDF)(*si, 1.f);
        BxDF *tr = ARENA_ALLOC(arena, SpecularTransmission)(t, 1.f, 1.f, mode);
        si->bsdf->Add(tr);
    } else
        si->bsdf = ARENA_ALLOC(arena, BSDF)(*si, e);

    Spectrum kd = op * Kd->Evaluate(*si).Clamp();
    if (!kd.IsBlack()) {
        BxDF *diff = ARENA_ALLOC(arena, LambertianReflection)(kd);
        si->bsdf->Add(diff);
    }

    Spectrum ks = op * Ks->Evaluate(*si).Clamp();
    if (!ks.IsBlack()) {
        Fresnel *fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, e);
        Float roughu, roughv;
        if (roughnessu)
            roughu = roughnessu->Evaluate(*si);
        else
            roughu = roughness->Evaluate(*si);
        if (roughnessv)
            roughv = roughnessv->Evaluate(*si);
        else
            roughv = roughu;
        if (remapRoughness) {
            roughu = TrowbridgeReitzDistribution::RoughnessToAlpha(roughu);
            roughv = TrowbridgeReitzDistribution::RoughnessToAlpha(roughv);
        }
        MicrofacetDistribution *distrib =
            ARENA_ALLOC(arena, TrowbridgeReitzDistribution)(roughu, roughv);
        BxDF *spec =
            ARENA_ALLOC(arena, MicrofacetReflection)(ks, distrib, fresnel);
        si->bsdf->Add(spec);
    }

    Spectrum kr = op * Kr->Evaluate(*si).Clamp();
    if (!kr.IsBlack()) {
        Fresnel *fresnel = ARENA_ALLOC(arena, FresnelDielectric)(1.f, e);
        si->bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(kr, fresnel));
    }

    Spectrum kt = op * Kt->Evaluate(*si).Clamp();
    if (!kt.IsBlack())
        si->bsdf->Add(
            ARENA_ALLOC(arena, SpecularTransmission)(kt, 1.f, e, mode));
}

RGBSpectrum UberMaterial::GetAlbedo(SurfaceInteraction *si) const {
    return opacity->Evaluate(*si).Clamp() * Material::GetAlbedo(si);
}

void UberMaterial::GetLUTReducibilities(
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
    if (Kr->IsConstant()) {
        LUT_SET_REDUCIBILITY(3)
    }
    if (Kt->IsConstant()) {
        LUT_SET_REDUCIBILITY(4)
    }
    if ((roughnessu && roughnessu->IsConstant()) ||
        (!roughnessu && roughness->IsConstant())) {
        LUT_SET_REDUCIBILITY(5)
    }
    if ((roughnessv && roughnessv->IsConstant()) ||
        (!roughnessv && roughness->IsConstant())) {
        LUT_SET_REDUCIBILITY(6)
    }
    if (eta->IsConstant()) {
        LUT_SET_REDUCIBILITY(7)
    }
}

void UberMaterial::GetLUTReductionIndices(
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
    if (Kr->IsConstant()) {
        const RGBSpectrum krMapped = Kr->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(3, krMapped)
    }
    if (Kt->IsConstant()) {
        const RGBSpectrum ktMapped = Kt->Evaluate().Clamp(0.f, 1.f);
        LUT_SET_INDICES_SPECTRUM(4, ktMapped)
    }
    if ((roughnessu && roughnessu->IsConstant()) ||
        (!roughnessu && roughness->IsConstant())) {
        Float roughU;
        if (roughnessu)
            roughU = roughnessu->Evaluate();
        else
            roughU = roughness->Evaluate();
        if (remapRoughness)
            roughU = TrowbridgeReitzDistribution::RoughnessToAlpha(roughU);
        const Float roughnessUMapped = Clamp(InverseLerp(roughU, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(5, roughnessUMapped)
    }
    if ((roughnessv && roughnessv->IsConstant()) ||
        (!roughnessv && roughness->IsConstant())) {
        Float roughV;
        if (roughnessv)
            roughV = roughnessv->Evaluate();
        else
            roughV = roughness->Evaluate();
        if (remapRoughness)
            roughV = TrowbridgeReitzDistribution::RoughnessToAlpha(roughV);
        const Float roughnessVMapped = Clamp(InverseLerp(roughV, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(6, roughnessVMapped)
    }
    if (eta->IsConstant()) {
        const Float etaMapped = Clamp(InverseLerp(eta->Evaluate(), 1.f + Epsilon, 2.42f), 0.f, 1.f);
        LUT_SET_INDICES(7, etaMapped)
    }
}

void UberMaterial::GetLUTIndices(
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
    if (!((roughnessu && roughnessu->IsConstant()) ||
          (!roughnessu && roughness->IsConstant()))) {
        Float roughU;
        if (roughnessu)
            roughU = roughnessu->Evaluate(*si);
        else
            roughU = roughness->Evaluate(*si);
        if (remapRoughness)
            roughU = TrowbridgeReitzDistribution::RoughnessToAlpha(roughU);
        const Float roughnessUMapped = Clamp(InverseLerp(roughU, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, roughnessUMapped)
        i++;
    }
    if (!((roughnessv && roughnessv->IsConstant()) ||
          (!roughnessv && roughness->IsConstant()))) {
        Float roughV;
        if (roughnessv)
            roughV = roughnessv->Evaluate(*si);
        else
            roughV = roughness->Evaluate(*si);
        if (remapRoughness)
            roughV = TrowbridgeReitzDistribution::RoughnessToAlpha(roughV);
        const Float roughnessVMapped = Clamp(InverseLerp(roughV, TrowbridgeAlphaMin, TrowbridgeAlphaMax), 0.f, 1.f);
        LUT_SET_INDICES(i, roughnessVMapped)
        i++;
    }
    if (!eta->IsConstant()) {
        const Float etaMapped = Clamp(InverseLerp(eta->Evaluate(*si), 1.f + Epsilon, 2.42f), 0.f, 1.f);
        LUT_SET_INDICES(i, etaMapped)
    }
}

UberMaterial *CreateUberMaterial(const TextureParams &mp,
                                 const unsigned long long id) {
    std::shared_ptr<Texture<Spectrum>> Kd =
        mp.GetSpectrumTexture("Kd", Spectrum(0.25f));
    std::shared_ptr<Texture<Spectrum>> Ks =
        mp.GetSpectrumTexture("Ks", Spectrum(0.25f));
    std::shared_ptr<Texture<Spectrum>> Kr =
        mp.GetSpectrumTexture("Kr", Spectrum(0.f));
    std::shared_ptr<Texture<Spectrum>> Kt =
        mp.GetSpectrumTexture("Kt", Spectrum(0.f));
    std::shared_ptr<Texture<Float>> roughness =
        mp.GetFloatTexture("roughness", .1f);
    std::shared_ptr<Texture<Float>> uroughness =
        mp.GetFloatTextureOrNull("uroughness");
    std::shared_ptr<Texture<Float>> vroughness =
        mp.GetFloatTextureOrNull("vroughness");
    std::shared_ptr<Texture<Float>> eta = mp.GetFloatTextureOrNull("eta");
    if (!eta) eta = mp.GetFloatTexture("index", 1.5f);
    std::shared_ptr<Texture<Spectrum>> opacity =
        mp.GetSpectrumTexture("opacity", 1.f);
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    bool remapRoughness = mp.FindBool("remaproughness", true);
    return new UberMaterial(Kd, Ks, Kr, Kt, roughness, uroughness, vroughness,
                            opacity, eta, bumpMap, remapRoughness, id);
}

}  // namespace pbrt
