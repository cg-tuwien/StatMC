// © 2024-2025 Hiroyuki Sakai

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_STATISTICS_STATPATHINTEGRATOR_H
#define PBRT_STATISTICS_STATPATHINTEGRATOR_H

#include "pbrt.h"
#include "integrator.h"
#include "lightdistrib.h"
#include "statistics/statpbrt.h"
#include "statistics/estimator.h"

namespace pbrt {

// We hardcode the vector indices here so that no expensive lookups must be performed during rendering.
enum BufferIndex {
    MaterialID = 0, // Float buffer vector indices
    Depth      = 1,
    Normal     = 0, // RGB buffer vector indices
    Albedo     = 1
};

enum StatTypeIndex {
    Radiance        = 0,
    MISBSDFWinRate  = 1,
    MISLightWinRate = 2,
    StatMaterialID  = 3,
    StatDepth       = 4,
    StatNormal      = 5,
    StatAlbedo      = 6,
    ItRadiance      = 7
};

struct MISTally {
    unsigned char bsdf  = 0;
    unsigned char light = 0;
};

struct MISWinRate {
    Float bsdf  = 0;
    Float light = 0;
};

class StatPathIntegrator : public SamplerIntegrator {
    protected:
        struct Features {
            std::vector<Float> floats;
            std::vector<Spectrum> spectrums;
        };

    public:
        StatPathIntegrator(
            const unsigned int maxDepth,
            std::shared_ptr<const Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const Bounds2i &pixelBounds,
            const uint64_t nIterations,
            const bool expIterations,
            const bool enableMultiChannelStats,
            const bool enableACRR,
            const bool denoiseImage,
            const bool enableSMIS,
            const bool calculateItStats,
            const float filterSD,
            const unsigned char filterRadius,
            GBufferConfigs floatGBufferConfigs,
            GBufferConfigs rgbGBufferConfigs,
            StatTypeConfigs statTypeConfigs,
            const Float rrThreshold = 1.f,
            const std::string &lightSampleStrategy = "spatial",
            const std::string &outputRegex = "film.*"
        );
        void Preprocess(const Scene &scene, Sampler &sampler);
        void WarmUp();
        // Templated function pointer typedef
        template <typename T>
        using AddSampleFn = void (StatTile<T>::*)(const Point2i p, const T sample);
        template <typename T>
        inline AddSampleFn<T> GetAddSampleFn(const StatTypeConfig &cfg);
        void Render(const Scene &scene);
        template <typename T>
        void Render(const Scene &scene);
        inline void ReadFile(const std::string &filename, Buffer &buffer);
        void Denoise(const Scene &scene);
        template <typename T>
        void Denoise(const Scene &scene);
        virtual Spectrum Li(
            const RayDifferential &ray,
            const Scene &scene,
            Sampler &sampler,
            MemoryArena &arena,
            Features &features,
            std::vector<Float> &avgLs,
            std::vector<MISWinRate> &misWinRates,
            std::vector<Spectrum> &L,
            std::vector<MISTally> &misTallies,
            unsigned int it
        ) const;

    protected:
        const GBufferConfigs floatGBufferConfigs;
        const GBufferConfigs rgbGBufferConfigs;
        const StatTypeConfigs statTypeConfigs;

        BufferRegistry bufferReg; /* Must be initialized before estimator!! */
        Estimator estimator;

    private:
        inline Float GetY(const Float val) const {return val;}
        inline Float GetY(const Vec3 rgb) const {
            Spectrum spectrum = Spectrum::FromRGB(&rgb[0]);
            return spectrum.y();
        }
        template <typename T>
        inline T GetStatSample(const Spectrum &spectrum) const;

        const int maxDepth;
        const Float rrThreshold;
        const std::string lightSampleStrategy;
        std::unique_ptr<LightDistribution> lightDistribution;

        const uint64_t nIterations;
        const bool expIterations;
        const bool enableMultiChannelStats;
        const bool denoiseImage;
        const bool enableACRR;
        const bool enableSMIS;
        const bool calculateItStats;

        unsigned char nFloatBuffers = 0;
        unsigned char nRGBBuffers = 0;

        const std::string outputRegex;
};

template <>
inline Float StatPathIntegrator::GetStatSample(const Spectrum &spectrum) const {return spectrum.y();};
template <>
inline Vec3  StatPathIntegrator::GetStatSample(const Spectrum &spectrum) const {
    Float rgb[3];
    spectrum.ToRGB(rgb);
    return Vec3(rgb);
}

StatPathIntegrator *CreateStatPathIntegrator(
    const ParamSet &params,
    const ParamSet &extraParams,
    std::shared_ptr<Sampler> sampler,
    std::shared_ptr<const Camera> camera
);

}  // namespace pbrt

#endif  // PBRT_STATISTICS_STATPATHINTEGRATOR_H
