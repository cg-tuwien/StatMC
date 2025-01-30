// © 2024-2025 Hiroyuki Sakai

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_STATISTICS_ESTIMATOR_H
#define PBRT_STATISTICS_ESTIMATOR_H

#include <unordered_set>

#include "pbrt.h"
#include "core/film.h"
#include "statistics/statpbrt.h"
#include "statistics/buffer.h"

namespace pbrt {

struct GBufferConfig {
    GBufferConfig(const std::string name = "") : name(name) {};
    std::string name;
    unsigned char index;
    bool enable = false;
};

struct GBufferConfigs {
    GBufferConfigs(
        std::vector<GBufferConfig> cfgs
    ) : configs(std::move(cfgs))
    {}
    unsigned char nEnabled = 0;
    std::vector<GBufferConfig> configs;
};

template <typename T>
class Tile {
    public:
        Tile(const Bounds2i &pixelBounds)
          : pixelBounds(pixelBounds)
        {
            pixels = std::vector<T>(std::max(0, pixelBounds.Area()));
        }
        T &GetPixel(const Point2i &p) {
            // CHECK(InsideExclusive(p, pixelBounds)); // Getting rid of these checks significantly increases performance (note that CHECK is executed in release mode)
            return pixels[(p.y - pixelBounds.pMin.y) * (pixelBounds.pMax.x - pixelBounds.pMin.x) +
                          (p.x - pixelBounds.pMin.x)];
        }
        const T &GetPixel(const Point2i &p) const {
            // CHECK(InsideExclusive(p, pixelBounds)); // Getting rid of these checks significantly increases performance (note that CHECK is executed in release mode)
            return pixels[(p.y - pixelBounds.pMin.y) * (pixelBounds.pMax.x - pixelBounds.pMin.x) +
                          (p.x - pixelBounds.pMin.x)];
        }
        Bounds2i GetPixelBounds() const { return pixelBounds; }

    protected:
        const Bounds2i pixelBounds;
        std::vector<T> pixels;
};

enum BufferGroupIndex {
    F1 = 0,
    F3 = 1,
    G1 = 2,
    G3 = 3
};

enum CUDAGroupIndex {
    DenoiseGroup = 0,
    CalculateMeanVarianceGroup = 1
};
static constexpr unsigned char nCUDAGroupIndices = 2;

struct StatTypeConfig {
    unsigned char type;
    unsigned char index;
    bool enable = false;
    unsigned char nBounces = 0;
    unsigned char bounceStart = 0;
    unsigned char bounceEnd = 0;
    unsigned char nChannels = 1;
    bool transform = false;
    unsigned char maxMoment = 1;

    bool gBuffer = false;
    bool enableForFilter = false;
    Float filterSD;

    std::vector<unsigned char> cudaGroups = {};
};

struct StatTypeConfigs {
    StatTypeConfig &operator[](size_t i) {
        return configs[i];
    };
    const StatTypeConfig &operator[](size_t i) const {
        return configs[i];
    };

    unsigned char nEnabled = 0;
    std::vector<StatTypeConfig> configs;
};

#ifdef PBRT_FLOAT_AS_DOUBLE
template <typename T>
struct StatTilePixel { // Float Vec3
    uint64_t n = 0;    //     8    8
    T mean = 0.;       //    16   32
    T m2   = 0.;       //    24   56
    T m3   = 0.;       //    32   80
    T filmMean = 0.;   //    40  104
    T filmM2   = 0.;   //    48  128
} __attribute__((aligned(64)));
#else
template <typename T>
struct StatTilePixel { // Float Vec3
    uint64_t n = 0;    //     8    8
    T mean = 0.f;      //    12   20
    T m2   = 0.f;      //    16   32 
    T m3   = 0.f;      //    20   44
    T filmMean = 0.f;  //    24   56
    T filmM2   = 0.f;  //    28   68
} __attribute__((aligned(64)));
#endif

// Needed for moment calculation below (definition in .cpp)
inline Vec3 operator*(const Vec3 &vec1, const Vec3 &vec2) {
    return Vec3(vec1[0] * vec2[0], vec1[1] * vec2[1], vec1[2] * vec2[2]);
}

inline Vec3 operator/(const Vec3 &vec1, const uint64_t &integer) {
    return Vec3(vec1[0] / integer, vec1[1] / integer, vec1[2] / integer);
}

inline Float boxCox(const Float &val, const Float &lambda) {
    return (std::pow(val, lambda) - 1.f) / lambda;
}

inline Vec3 boxCox(const Vec3 &vec, const Float &lambda) {
    return Vec3(
        (std::pow(vec[0], lambda) - 1.f) / lambda,
        (std::pow(vec[1], lambda) - 1.f) / lambda,
        (std::pow(vec[2], lambda) - 1.f) / lambda
    );
}

template <typename T>
class StatTile : public Tile<StatTilePixel<T>> {
    public:
        StatTile(const Bounds2i &pixelBounds) : Tile<StatTilePixel<T>>(pixelBounds), filterTable(nullptr), filterTableSize(0)
        {}
        StatTile(
            const Bounds2i &pixelBounds, const Vector2f &filterRadius,
            const Float *filterTable, int filterTableSize
        ) : Tile<StatTilePixel<T>>(pixelBounds),
            pixelBounds(pixelBounds),
            filterRadius(filterRadius),
            invFilterRadius(1 / filterRadius.x, 1 / filterRadius.y),
            filterTable(filterTable),
            filterTableSize(filterTableSize)
        {}
        inline void AddStatSampleM1(StatTilePixel<T> &pixel, const T sample) {
            uint64_t &n = pixel.n;

            T &mean = pixel.mean;

            // Use Meng's algorithm (https://arxiv.org/abs/1510.04923)
            n++;
            const T d   = sample - mean;
            const T dN  = d / n;

            mean += dN;
        }
        inline void AddStatSampleM2(StatTilePixel<T> &pixel, const T sample) {
            uint64_t &n = pixel.n;

            T &mean = pixel.mean;
            T &m2   = pixel.m2;

            // Use Meng's algorithm (https://arxiv.org/abs/1510.04923)
            n++;
            const T d   = sample - mean;
            const T dN  = d / n;

            mean += dN;
            m2   += d * (d - dN);
        }
        inline void AddStatSampleM3(StatTilePixel<T> &pixel, const T sample) {
            uint64_t &n = pixel.n;

            T &mean = pixel.mean;
            T &m2   = pixel.m2;
            T &m3   = pixel.m3;

            // Use Meng's algorithm (https://arxiv.org/abs/1510.04923)
            n++;
            const T d   = sample - mean;
            const T d2  = d * d;
            const T dN  = d / n;
            const T dN2 = dN * dN;

            mean += dN;
            m2   +=                    d * (d      - dN      );
            m3   += - 3.f * dN  * m2 + d * (d2     - dN2     );
        }
        void AddSample(const Point2i p, const T sample, void (StatTile::*fn)(StatTilePixel<T> &pixel, const T sample)) {
            StatTilePixel<T> &pixel = this->GetPixel(p);
            (this->*fn)(pixel, sample);
            pixel.filmMean = pixel.mean;
            pixel.filmM2 = pixel.m2;
        }
        void AddTransformSample(const Point2i p, const T sample, void (StatTile::*fn)(StatTilePixel<T> &pixel, const T sample)) {
            StatTilePixel<T> &pixel = this->GetPixel(p);
            
            (this->*fn)(pixel, boxCox(sample, .5f));

            T &filmMean = pixel.filmMean;
            T &filmM2   = pixel.filmM2;

            const T filmD  = sample - filmMean;
            const T filmD2 = filmD * filmD;
            const T filmDN = filmD / pixel.n;

            filmMean += filmDN;
            filmM2   += filmD * (filmD - filmDN);
        }
        void AddSampleM1         (const Point2i p, const T sample) { AddSample         (p, sample, &StatTile<T>::AddStatSampleM1); }
        void AddTransformSampleM1(const Point2i p, const T sample) { AddTransformSample(p, sample, &StatTile<T>::AddStatSampleM1); }
        void AddSampleM2         (const Point2i p, const T sample) { AddSample         (p, sample, &StatTile<T>::AddStatSampleM2); }
        void AddTransformSampleM2(const Point2i p, const T sample) { AddTransformSample(p, sample, &StatTile<T>::AddStatSampleM2); }
        void AddSampleM3         (const Point2i p, const T sample) { AddSample         (p, sample, &StatTile<T>::AddStatSampleM3); }
        void AddTransformSampleM3(const Point2i p, const T sample) { AddTransformSample(p, sample, &StatTile<T>::AddStatSampleM3); }

    private:
        const Bounds2i pixelBounds;
        const Vector2f filterRadius, invFilterRadius;
        const Float *filterTable;
        const int filterTableSize;
};

class Estimator {
    public:
        Estimator(
            const Buffer &filmBuffer,
            const StatTypeConfigs &statTypeConfigs,
            const float filterSD,
            const unsigned char filterRadius,
            const bool denoiseFilm,
            const bool acrrEnabled,
            const bool smisEnabled,
            const uint64_t samplesPerPixel,
            BufferRegistry &reg,
            const Bounds2i &croppedPixelBounds,
            std::shared_ptr<Filter> filt
        ) : filmBuffer(filmBuffer),
            filmFilteredBuffer("film-f", Mat3(filmBuffer.mat.rows, filmBuffer.mat.cols)),
            width(filmBuffer.mat.cols),
            height(filmBuffer.mat.rows),
            filterDSFactor(-.5f / (filterSD * filterSD)),
            filterRadius(filterRadius),
            denoiseFilm(denoiseFilm),
            acrrEnabled(acrrEnabled),
            smisEnabled(smisEnabled),
            croppedPixelBounds(croppedPixelBounds),
            filter(std::move(filt))
        {
            floatBufferCounts = std::vector<unsigned char>(nCUDAGroupIndices, 0);
            rgbBufferCounts = std::vector<unsigned char>(nCUDAGroupIndices, 0);

            std::copy_if(statTypeConfigs.configs.begin(), statTypeConfigs.configs.end(), std::back_inserter(this->statTypeConfigs.configs), [](StatTypeConfig cfg){ return cfg.enable; }); // This statTypeConfigs only holds statTypeConfigs for enabled buffers!
            this->statTypeConfigs.nEnabled = this->statTypeConfigs.configs.size();

            reg.Register(filmFilteredBuffer);

            if (denoiseFilm) {
                uploadBuffers.insert(&this->filmBuffer);
                downloadBuffers.insert(&this->filmFilteredBuffer);
            }

            cv::cuda::stat_denoiser::setup();
        }
        void RegisterGBuffer(Buffer &b, const Float filterSD);
        void AllocateBuffers(BufferRegistry &reg);
        template <typename T>
        std::vector<StatTile<T>> GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const;
        template <typename T>
        std::vector<std::vector<StatTile<T>>> GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const;
        template <typename T>
        std::vector<StatTile<T>> GetTilesF(const Bounds2i &sampleBounds, const unsigned char bounceEnd) const;
        template <typename T>
        std::vector<std::vector<StatTile<T>>> GetTilesF(const Bounds2i &sampleBounds, const unsigned char bounceEnd, const unsigned char n) const;
        template <typename T>
        inline void MergeTile(const StatTile<T> &tile, const unsigned char statTypeIndex, const unsigned char bounceIndex) const;
        template <typename T>
        void MergeTiles(const std::vector<StatTile<T>> &tiles, const StatTypeConfig &cfg) const;
        template <typename T>
        void MergeTiles(const std::vector<std::vector<StatTile<T>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const;
        template <typename T>
        inline void MergeTransformTile(const StatTile<T> &tile, const unsigned char statTypeIndex, const unsigned char bounceIndex) const;
        template <typename T>
        void MergeTransformTiles(const std::vector<StatTile<T>> &tiles, const StatTypeConfig &cfg) const;
        template <typename T>
        void MergeTransformTiles(const std::vector<std::vector<StatTile<T>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const;
        void Upload();
        void Download();
        void Denoise();
        void CalculateMeanVars();
        void Synchronize();


        const unsigned short width;
        const unsigned short height;
        std::shared_ptr<Filter> filter;
        Bounds2i croppedPixelBounds;

        const float filterDSFactor;
        const unsigned char filterRadius;
        const bool denoiseFilm;
        const bool acrrEnabled;
        const bool smisEnabled;

        std::vector<unsigned char> floatBufferCounts;
        std::vector<unsigned char> rgbBufferCounts;
        bool runCUDA = false;

        cv::cuda::Stream stream;

        Buffer filmBuffer;
        Buffer filmFilteredBuffer;

        StatTypeConfigs statTypeConfigs;

        std::unordered_set<Buffer *> uploadBuffers;
        std::unordered_set<Buffer *> downloadBuffers;

        std::vector<std::vector<Buffer>> nBuffers; // unsigned int and long are not supported by Mat as of writing this code.
        std::vector<std::vector<Buffer>> meanBuffers;
        std::vector<std::vector<Buffer>> m2Buffers;
        std::vector<std::vector<Buffer>> m3Buffers;
        std::vector<GpuMat> nFGPUPtrs;
        std::vector<GpuMat> meanFGPUPtrs;
        std::vector<GpuMat> m2FGPUPtrs;
        std::vector<GpuMat> m3FGPUPtrs;
        std::vector<GpuMat> nRGBGPUPtrs;
        std::vector<GpuMat> meanRGBGPUPtrs;
        std::vector<GpuMat> m2RGBGPUPtrs;
        std::vector<GpuMat> m3RGBGPUPtrs;

        std::vector<std::vector<Buffer>> filmBuffers;
        std::vector<std::vector<Buffer>> filmM2Buffers;
        std::vector<std::vector<Buffer>> filmFilteredBuffers;
        std::vector<std::vector<Buffer>> filmVarBuffers;
        std::vector<GpuMat> filmFGPUPtrs;
        std::vector<GpuMat> filmM2FGPUPtrs;
        std::vector<GpuMat> filmFilteredFGPUPtrs;
        std::vector<GpuMat> filmVarFGPUPtrs;
        std::vector<GpuMat> filmRGBGPUPtrs;
        std::vector<GpuMat> filmM2RGBGPUPtrs;
        std::vector<GpuMat> filmFilteredRGBGPUPtrs;
        std::vector<GpuMat> filmVarRGBGPUPtrs;

        std::vector<Buffer> gBuffers;
        std::vector<Float>  gBufferDRFactors;
        GpuMat gBufferGPUPtrs;
        GpuMat gBufferChannelCountsGPUMat;
        GpuMat gBufferDRFactorsGPUMat;

        std::vector<std::vector<Buffer>> meanCorrBuffers;
        std::vector<std::vector<Buffer>> discriminatorBuffers;
        std::vector<GpuMat> meanCorrFGPUPtrs;
        std::vector<GpuMat> discriminatorFGPUPtrs;
        std::vector<GpuMat> meanCorrRGBGPUPtrs;
        std::vector<GpuMat> discriminatorRGBGPUPtrs;

    protected:
        // Film Protected Data
        static PBRT_CONSTEXPR int filterTableWidth = 16;
        Float filterTable[filterTableWidth * filterTableWidth];
};

}  // namespace pbrt

#endif  // PBRT_STATISTICS_ESTIMATOR_H
