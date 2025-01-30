// © 2024-2025 Hiroyuki Sakai

#include "statistics/estimator.h"
#include <opencv2/cudaimgproc.hpp>
#include "spectrum.h"
#include "statistics/statpath.h"

struct float3 {
    float x, y, z;
};

namespace pbrt {

void Estimator::RegisterGBuffer(Buffer &b, const Float filterSD) {
    gBuffers.push_back(b);
    gBufferDRFactors.emplace_back(-.5f / (filterSD * filterSD));
}


// Using #defines here for simplicity, even though it might not be the most elegant solution.
#define APPEND_BUFFER_VEC(buffers) { \
    buffers.emplace_back().reserve(cfg.nBounces); \
} \

#define ALLOC_BUFFER(buffers, suffix, mat) { \
    Buffer &b = buffers[cfg.index].emplace_back("t" + iStr + "-b" + jStr + suffix, mat); \
    reg.Register(b); \
} \

#define ALLOC_BUFFER_GPU(buffers, suffix, mat, gpuMat) { \
    Buffer &b = buffers[cfg.index].emplace_back("t" + iStr + "-b" + jStr + suffix, mat, gpuMat); \
    reg.Register(b); \
} \

#define PREPARE_STAT_BUFFER_GPU_PTRS(buffers, fPtrs, rgbPtrs) { \
    std::vector<Mat> fPtrsCPUs(nCUDAGroupIndices); \
    std::vector<PtrStepSzb *> fPtrsCPUPtrs(nCUDAGroupIndices); \
    std::vector<Mat> rgbPtrsCPUs(nCUDAGroupIndices); \
    std::vector<PtrStepSzb *> rgbPtrsCPUPtrs(nCUDAGroupIndices); \
    for (unsigned char i = 0; i < nCUDAGroupIndices; i++) { \
        fPtrsCPUs[i] = Mat(1, floatBufferCounts[i], CV_8UC(sizeof(PtrStepSzb))); \
        fPtrsCPUPtrs[i] = fPtrsCPUs[i].ptr<PtrStepSzb>(); \
        rgbPtrsCPUs[i] = Mat(1, rgbBufferCounts[i], CV_8UC(sizeof(PtrStepSzb))); \
        rgbPtrsCPUPtrs[i] = rgbPtrsCPUs[i].ptr<PtrStepSzb>(); \
    } \
    for (unsigned char i = 0; i < cfgs.nEnabled; i++) \
        if (cfgs.configs[i].nChannels == 3) \
            for (unsigned char k : cfgs.configs[i].cudaGroups) \
                if (k != CalculateMeanVarianceGroup) \
                    for (unsigned char j = 0; j < cfgs.configs[i].nBounces; j++, rgbPtrsCPUPtrs[k]++) \
                        *rgbPtrsCPUPtrs[k] = buffers[i][j].gpuMat; \
                else { \
                    *rgbPtrsCPUPtrs[k] = buffers[i][0].gpuMat; \
                    rgbPtrsCPUPtrs[k]++; \
                } \
        else \
            for (unsigned char k : cfgs.configs[i].cudaGroups) \
                if (k != CalculateMeanVarianceGroup) \
                    for (unsigned char j = 0; j < cfgs.configs[i].nBounces; j++, fPtrsCPUPtrs[k]++) \
                        *fPtrsCPUPtrs[k] = buffers[i][j].gpuMat; \
                else { \
                    *fPtrsCPUPtrs[k] = buffers[i][0].gpuMat; \
                    fPtrsCPUPtrs[k]++; \
                } \
    for (unsigned char i = 0; i < nCUDAGroupIndices; i++) { \
        fPtrs[i].upload(fPtrsCPUs[i], stream); \
        rgbPtrs[i].upload(rgbPtrsCPUs[i], stream); \
    } \
}

// *ptrsCPUPtr = mats[i] is an implicit conversion of GPUMat to PtrStepSzb, which can be addressed in the kernel.
#define PREPARE_G_BUFFER_GPU_PTRS(buffers, ptrs, channelCounts) { \
    unsigned char size = buffers.size(); \
    Mat ptrsCPU(1, size, CV_8UC(sizeof(PtrStepSzb))); \
    Mat channelCountsCPU(1, size, CV_8UC1); \
    PtrStepSzb *ptrsCPUPtr = ptrsCPU.ptr<PtrStepSzb>(); \
    unsigned char *channelCountsCPUPtr = channelCountsCPU.ptr<unsigned char>(); \
    for (unsigned char i = 0; i < size; i++, ptrsCPUPtr++, channelCountsCPUPtr++) { \
        *ptrsCPUPtr = buffers[i].gpuMat; \
        *channelCountsCPUPtr = buffers[i].gpuMat.channels(); \
    } \
    ptrs.upload(ptrsCPU, stream); \
    channelCounts.upload(channelCountsCPU, stream); \
} \

void Estimator::AllocateBuffers(BufferRegistry &reg) {
    auto &cfgs = statTypeConfigs;

    nBuffers.reserve(cfgs.nEnabled);
    meanBuffers.reserve(cfgs.nEnabled);
    m2Buffers.reserve(cfgs.nEnabled);
    m3Buffers.reserve(cfgs.nEnabled);
    meanCorrBuffers.reserve(cfgs.nEnabled);
    discriminatorBuffers.reserve(cfgs.nEnabled);

    filmBuffers.reserve(cfgs.nEnabled);
    filmFilteredBuffers.reserve(cfgs.nEnabled);
    filmM2Buffers.reserve(cfgs.nEnabled);
    filmVarBuffers.reserve(cfgs.nEnabled);

    for (unsigned char i = 0; i < cfgs.nEnabled; i++) {
        auto &cfg = cfgs.configs[i];

        APPEND_BUFFER_VEC(nBuffers)
        APPEND_BUFFER_VEC(meanBuffers)
        APPEND_BUFFER_VEC(m2Buffers)
        APPEND_BUFFER_VEC(m3Buffers)
        APPEND_BUFFER_VEC(meanCorrBuffers)
        APPEND_BUFFER_VEC(discriminatorBuffers)

        APPEND_BUFFER_VEC(filmBuffers)
        APPEND_BUFFER_VEC(filmFilteredBuffers)
        APPEND_BUFFER_VEC(filmM2Buffers)
        APPEND_BUFFER_VEC(filmVarBuffers)

        for (unsigned char j = cfg.bounceStart; j < cfg.bounceEnd; j++) {
            std::string iStr = std::to_string(i);
            std::string jStr = std::to_string(j);

            if (cfg.nChannels == 3) {
                ALLOC_BUFFER(nBuffers, "-n", Mat_<int>  (height, width))
                if (cfg.transform) {
                    ALLOC_BUFFER(meanBuffers,   "-mean",      Mat_<Vec3>(height, width))
                    ALLOC_BUFFER(m2Buffers,     "-m2",        Mat_<Vec3>(height, width))
                    ALLOC_BUFFER(filmBuffers,   "-film-mean", Mat_<Vec3>(height, width))
                    ALLOC_BUFFER(filmM2Buffers, "-film-m2",   Mat_<Vec3>(height, width))
                } else {
                    // m2 and mean point to their film counterparts in case of no transformation
                    Mat mean = Mat_<Vec3>(height, width);
                    Mat m2   = Mat_<Vec3>(height, width);
                    GpuMat meanGPU = GpuMat(height, width, mean.type());
                    GpuMat m2GPU   = GpuMat(height, width, m2.type());
                    ALLOC_BUFFER_GPU(meanBuffers,   "-mean",      mean, meanGPU)
                    ALLOC_BUFFER_GPU(m2Buffers,     "-m2",        m2,   m2GPU)
                    ALLOC_BUFFER_GPU(filmBuffers,   "-film-mean", mean, meanGPU)
                    ALLOC_BUFFER_GPU(filmM2Buffers, "-film-m2",   m2,   m2GPU)
                }
                ALLOC_BUFFER(m3Buffers, "-m3", Mat_<Vec3>(height, width))

                ALLOC_BUFFER(meanCorrBuffers,      "-mean-corr",     Mat_<Vec3>(height, width))
                ALLOC_BUFFER(discriminatorBuffers, "-discriminator", Mat_<Vec3>(height, width))
                ALLOC_BUFFER(filmVarBuffers,       "-film-mean-var", Mat_<Vec3>(height, width))
                if (denoiseFilm && cfg.type == Radiance && j == 0)
                    ALLOC_BUFFER(filmFilteredBuffers, "-film-mean-f", filmFilteredBuffer.mat)
                else
                    ALLOC_BUFFER(filmFilteredBuffers, "-film-mean-f", Mat_<Vec3>(height, width))

                if (cfg.gBuffer) {
                    if (cfg.enableForFilter) {
                        RegisterGBuffer(filmBuffers[i][j], cfg.filterSD);
                        uploadBuffers.insert(&filmBuffers[i][j]);
                    }
                }

                for (unsigned char k : cfg.cudaGroups)
                    if (k != CalculateMeanVarianceGroup) {
                        rgbBufferCounts[k]++;
                        runCUDA = true;
                    } else if (j == 0) { // Estimator variance estimate is only interesting for zeroth bounce and is calculated on the CPU instead of the GPU
                        rgbBufferCounts[k]++;
                    }

                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), DenoiseGroup) != cfg.cudaGroups.end()) {
                    uploadBuffers.insert(&nBuffers[i][j]);
                    uploadBuffers.insert(&meanBuffers[i][j]);
                    uploadBuffers.insert(&m2Buffers[i][j]);
                    uploadBuffers.insert(&m3Buffers[i][j]);
                    if (!(denoiseFilm && cfg.type == Radiance && j == 0)) { // Skip up/downloading radiance buffer at zeroth bounce because that's already covered by the film buffer
                        if (cfg.transform)
                            uploadBuffers.insert(&filmBuffers[i][j]);
                        downloadBuffers.insert(&filmFilteredBuffers[i][j]);
                    }
                }
                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), CalculateMeanVarianceGroup) != cfg.cudaGroups.end() && j == 0) {
                    uploadBuffers.insert(&nBuffers[i][j]);
                    uploadBuffers.insert(&filmM2Buffers[i][j]);
                    downloadBuffers.insert(&filmVarBuffers[i][j]);
                }
            } else {
                ALLOC_BUFFER(nBuffers, "-n", Mat_<int>(height, width))
                if (cfg.transform) {
                    ALLOC_BUFFER(meanBuffers,   "-mean",      Mat_<Float>(height, width))
                    ALLOC_BUFFER(m2Buffers,     "-m2",        Mat_<Float>(height, width))
                    ALLOC_BUFFER(filmBuffers,   "-film-mean", Mat_<Float>(height, width))
                    ALLOC_BUFFER(filmM2Buffers, "-film-m2",   Mat_<Float>(height, width))
                } else {
                    // m2 and mean point to their film counterparts in case of no transformation
                    Mat mean = Mat_<Float>(height, width);
                    Mat m2   = Mat_<Float>(height, width);
                    GpuMat meanGPU = GpuMat(height, width, mean.type());
                    GpuMat m2GPU   = GpuMat(height, width, m2.type());
                    ALLOC_BUFFER_GPU(meanBuffers,   "-mean",      mean, meanGPU)
                    ALLOC_BUFFER_GPU(m2Buffers,     "-m2",        m2,   m2GPU)
                    ALLOC_BUFFER_GPU(filmBuffers,   "-film-mean", mean, meanGPU)
                    ALLOC_BUFFER_GPU(filmM2Buffers, "-film-m2",   m2,   m2GPU)

                }
                ALLOC_BUFFER(m3Buffers, "-m3", Mat_<Float>(height, width))
              
                ALLOC_BUFFER(meanCorrBuffers,      "-mean-corr",     Mat_<Float>(height, width))
                ALLOC_BUFFER(discriminatorBuffers, "-discriminator", Mat_<Float>(height, width))
                ALLOC_BUFFER(filmVarBuffers,       "-film-mean-var", Mat_<Float>(height, width))
                ALLOC_BUFFER(filmFilteredBuffers,  "-film-mean-f",   Mat_<Float>(height, width))

                if (cfg.gBuffer) {
                    if (cfg.enableForFilter) {
                        RegisterGBuffer(filmBuffers[i][j], cfg.filterSD);
                        uploadBuffers.insert(&filmBuffers[i][j]);
                    }
                }

                for (unsigned char k : cfg.cudaGroups)
                    if (k != CalculateMeanVarianceGroup) {
                        floatBufferCounts[k]++;
                        runCUDA = true;
                    } else if (j == 0) { // Estimator variance estimate is only interesting for zeroth bounce and is calculated on the CPU now instead of the GPU
                        floatBufferCounts[k]++;
                    }

                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), DenoiseGroup) != cfg.cudaGroups.end()) {
                    uploadBuffers.insert(&nBuffers[i][j]);
                    uploadBuffers.insert(&meanBuffers[i][j]);
                    uploadBuffers.insert(&m2Buffers[i][j]);
                    uploadBuffers.insert(&m3Buffers[i][j]);
                    if (acrrEnabled || smisEnabled) {
                        if (cfg.transform)
                            uploadBuffers.insert(&filmBuffers[i][j]);
                        downloadBuffers.insert(&filmFilteredBuffers[i][j]);
                    }
                }
                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), CalculateMeanVarianceGroup) != cfg.cudaGroups.end() && j == 0) {
                    uploadBuffers.insert(&nBuffers[i][j]);
                    uploadBuffers.insert(&filmM2Buffers[i][j]);
                    downloadBuffers.insert(&filmVarBuffers[i][j]);
                }
            }
        }
    }

    {
        using cv::cuda::PtrStepSzb;

        nFGPUPtrs   .resize(nCUDAGroupIndices);
        meanFGPUPtrs.resize(nCUDAGroupIndices);
        m2FGPUPtrs  .resize(nCUDAGroupIndices);
        m3FGPUPtrs  .resize(nCUDAGroupIndices);

        filmM2FGPUPtrs        .resize(nCUDAGroupIndices);

        meanCorrFGPUPtrs          .resize(nCUDAGroupIndices);
        discriminatorFGPUPtrs          .resize(nCUDAGroupIndices);
        filmVarFGPUPtrs     .resize(nCUDAGroupIndices);
        filmFGPUPtrs        .resize(nCUDAGroupIndices);
        filmFilteredFGPUPtrs.resize(nCUDAGroupIndices);


        nRGBGPUPtrs   .resize(nCUDAGroupIndices);
        meanRGBGPUPtrs.resize(nCUDAGroupIndices);
        m2RGBGPUPtrs  .resize(nCUDAGroupIndices);
        m3RGBGPUPtrs  .resize(nCUDAGroupIndices);

        filmM2RGBGPUPtrs        .resize(nCUDAGroupIndices);

        meanCorrRGBGPUPtrs          .resize(nCUDAGroupIndices);
        discriminatorRGBGPUPtrs          .resize(nCUDAGroupIndices);
        filmVarRGBGPUPtrs     .resize(nCUDAGroupIndices);
        filmRGBGPUPtrs        .resize(nCUDAGroupIndices);
        filmFilteredRGBGPUPtrs.resize(nCUDAGroupIndices);

        // These GPU pointers point to the already allocated GPU buffers and can be simply passed to the GPU for reading and writing.
        PREPARE_STAT_BUFFER_GPU_PTRS(nBuffers, nFGPUPtrs, nRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(meanBuffers, meanFGPUPtrs, meanRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(m2Buffers, m2FGPUPtrs, m2RGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(m3Buffers, m3FGPUPtrs, m3RGBGPUPtrs)

        PREPARE_STAT_BUFFER_GPU_PTRS(filmM2Buffers, filmM2FGPUPtrs, filmM2RGBGPUPtrs)
        
        PREPARE_STAT_BUFFER_GPU_PTRS(meanCorrBuffers, meanCorrFGPUPtrs, meanCorrRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(discriminatorBuffers, discriminatorFGPUPtrs, discriminatorRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(filmVarBuffers, filmVarFGPUPtrs, filmVarRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(filmBuffers, filmFGPUPtrs, filmRGBGPUPtrs)
        PREPARE_STAT_BUFFER_GPU_PTRS(filmFilteredBuffers, filmFilteredFGPUPtrs, filmFilteredRGBGPUPtrs)

        PREPARE_G_BUFFER_GPU_PTRS(gBuffers, gBufferGPUPtrs, gBufferChannelCountsGPUMat)
    }

    Mat gBufferDRFactorsMat(gBufferDRFactors);
    gBufferDRFactorsGPUMat.upload(gBufferDRFactorsMat, stream);
}

#undef APPEND_BUFFER_VEC
#undef ALLOC_BUFFER
#undef PREPARE_STAT_BUFFER_GPU_PTRS
#undef PREPARE_G_BUFFER_GPU_PTRS


template <typename T>
std::vector<StatTile<T>> Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const {
    return std::vector<StatTile<T>>(bounceEnd, StatTile<T>(tilePixelBounds));
}
template std::vector<StatTile<Float>> Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const;
template std::vector<StatTile<Vec3>>  Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const;

template <typename T>
std::vector<std::vector<StatTile<T>>> Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const {
    return std::vector<std::vector<StatTile<T>>>(bounceEnd, std::vector<StatTile<T>>(n, StatTile<T>(tilePixelBounds)));
}
template std::vector<std::vector<StatTile<Float>>> Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const;
template std::vector<std::vector<StatTile<Vec3>>>  Estimator::GetTiles(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const;


template <typename T>
std::vector<StatTile<T>> Estimator::GetTilesF(const Bounds2i &sampleBounds, const unsigned char bounceEnd) const {
    // Bound image pixels that samples in _sampleBounds_ contribute to
    Vector2f halfPixel = Vector2f(0.5f, 0.5f);
    Bounds2f floatBounds = (Bounds2f)sampleBounds;
    Point2i p0 = (Point2i)Ceil(floatBounds.pMin - halfPixel - filter->radius);
    Point2i p1 = (Point2i)Floor(floatBounds.pMax - halfPixel + filter->radius) +
                 Point2i(1, 1);
    Bounds2i tilePixelBounds = Intersect(Bounds2i(p0, p1), croppedPixelBounds);
    return std::vector<StatTile<T>>(bounceEnd, StatTile<T>(tilePixelBounds, filter->radius, filterTable, filterTableWidth));
}
template std::vector<StatTile<Float>> Estimator::GetTilesF(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const;
template std::vector<StatTile<Vec3>>  Estimator::GetTilesF(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd) const;

template <typename T>
std::vector<std::vector<StatTile<T>>> Estimator::GetTilesF(const Bounds2i &sampleBounds, const unsigned char bounceEnd, const unsigned char n) const {
    // Bound image pixels that samples in _sampleBounds_ contribute to
    Vector2f halfPixel = Vector2f(0.5f, 0.5f);
    Bounds2f floatBounds = (Bounds2f)sampleBounds;
    Point2i p0 = (Point2i)Ceil(floatBounds.pMin - halfPixel - filter->radius);
    Point2i p1 = (Point2i)Floor(floatBounds.pMax - halfPixel + filter->radius) +
                 Point2i(1, 1);
    Bounds2i tilePixelBounds = Intersect(Bounds2i(p0, p1), croppedPixelBounds);
    return std::vector<std::vector<StatTile<T>>>(bounceEnd, std::vector<StatTile<T>>(n, StatTile<T>(tilePixelBounds, filter->radius, filterTable, filterTableWidth)));
}
template std::vector<std::vector<StatTile<Float>>> Estimator::GetTilesF(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const;
template std::vector<std::vector<StatTile<Vec3>>>  Estimator::GetTilesF(const Bounds2i &tilePixelBounds, const unsigned char bounceEnd, const unsigned char n) const;


template <typename T>
inline void Estimator::MergeTile(const StatTile<T> &tile, const unsigned char statTypeIndex, const unsigned char bounceIndex) const {
    for (const Point2i p : tile.GetPixelBounds()) {
        const unsigned int offset = p.y * width + p.x;
        const StatTilePixel<T> &tilePixel = tile.GetPixel(p);

        ((int *) nBuffers   [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.n;
        ((T   *) meanBuffers[statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.mean;
        ((T   *) m2Buffers  [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.m2;
        ((T   *) m3Buffers  [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.m3;
    }
}

template <typename T>
void Estimator::MergeTiles(const std::vector<StatTile<T>> &tiles, const StatTypeConfig &cfg) const {
    for (unsigned char j = 0; j < cfg.nBounces; j++)
        MergeTile(tiles[j+cfg.bounceStart], cfg.index, j);
}
template void Estimator::MergeTiles(const std::vector<StatTile<Float>> &tiles, const StatTypeConfig &cfg) const;
template void Estimator::MergeTiles(const std::vector<StatTile<Vec3>>  &tiles, const StatTypeConfig &cfg) const;

template <typename T>
void Estimator::MergeTiles(const std::vector<std::vector<StatTile<T>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const {
    for (unsigned char i = 0; i < cfgs.size(); i++) {
        auto &cfg = cfgs[i];
        for (unsigned char j = 0; j < cfg.nBounces; j++)
            MergeTile(tiles[j+cfg.bounceStart][i], cfg.index, j);
    }
}
template void Estimator::MergeTiles(const std::vector<std::vector<StatTile<Float>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const;
template void Estimator::MergeTiles(const std::vector<std::vector<StatTile<Vec3>>>  &tiles, const std::vector<StatTypeConfig> &cfgs) const;


template <typename T>
inline void Estimator::MergeTransformTile(const StatTile<T> &tile, const unsigned char statTypeIndex, const unsigned char bounceIndex) const {
    for (const Point2i p : tile.GetPixelBounds()) {
        const unsigned int offset = p.y * width + p.x;
        const StatTilePixel<T> &tilePixel = tile.GetPixel(p);

        ((int *) nBuffers   [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.n;
        ((T   *) meanBuffers[statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.mean;
        ((T   *) m2Buffers  [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.m2;
        ((T   *) m3Buffers  [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.m3;

        ((T   *) filmBuffers[statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.filmMean;
        ((T   *) filmM2Buffers  [statTypeIndex][bounceIndex].matPtr)[offset] = tilePixel.filmM2;
    }
}

template <typename T>
void Estimator::MergeTransformTiles(const std::vector<StatTile<T>> &tiles, const StatTypeConfig &cfg) const {
    for (unsigned char j = 0; j < cfg.nBounces; j++)
        MergeTransformTile(tiles[j+cfg.bounceStart], cfg.index, j);
}
template void Estimator::MergeTransformTiles(const std::vector<StatTile<Float>> &tiles, const StatTypeConfig &cfg) const;
template void Estimator::MergeTransformTiles(const std::vector<StatTile<Vec3>>  &tiles, const StatTypeConfig &cfg) const;

template <typename T>
void Estimator::MergeTransformTiles(const std::vector<std::vector<StatTile<T>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const {
    for (unsigned char i = 0; i < cfgs.size(); i++) {
        auto &cfg = cfgs[i];
        for (unsigned char j = 0; j < cfg.nBounces; j++)
            MergeTransformTile(tiles[j+cfg.bounceStart][i], cfg.index, j);
    }
}
template void Estimator::MergeTransformTiles(const std::vector<std::vector<StatTile<Float>>> &tiles, const std::vector<StatTypeConfig> &cfgs) const;
template void Estimator::MergeTransformTiles(const std::vector<std::vector<StatTile<Vec3>>>  &tiles, const std::vector<StatTypeConfig> &cfgs) const;

void Estimator::Upload() {
    for (Buffer *b : uploadBuffers) {
#if DEBUG
        std::cout << "Uploading " << b->name << std::endl;
#endif
        b->upload(stream);
    }
}

void Estimator::Download() {
    for (Buffer *b : downloadBuffers) {
#if DEBUG
        std::cout << "Downloading " << b->name << std::endl;
#endif
        b->download(stream);
    }
}

void Estimator::Denoise() {
#if DEBUG
    std::cout << "Denoise()" << std::endl;
    std::cout << "  Float buffer count: " << (int) floatBufferCounts[DenoiseGroup] << std::endl;
    std::cout << "  RGB buffer count:   " << (int) rgbBufferCounts[DenoiseGroup] << std::endl;
#endif

    if (floatBufferCounts[DenoiseGroup] > 0) {
        auto &cfg = statTypeConfigs[Radiance];
        auto &cudaGroups = cfg.cudaGroups;
        cv::cuda::stat_denoiser::filter<float>(
            floatBufferCounts[DenoiseGroup],
            width,
            height,
            filterDSFactor,
            filterRadius,
            denoiseFilm,
            nFGPUPtrs[DenoiseGroup],
            meanFGPUPtrs[DenoiseGroup],
            m2FGPUPtrs[DenoiseGroup],
            m3FGPUPtrs[DenoiseGroup],
            filmFGPUPtrs[DenoiseGroup],
            filmBuffer.gpuMat,
            gBufferGPUPtrs,
            gBufferChannelCountsGPUMat,
            gBufferDRFactorsGPUMat,
            gBuffers.size(),
            meanCorrFGPUPtrs[DenoiseGroup],
            discriminatorFGPUPtrs[DenoiseGroup],
            filmFilteredFGPUPtrs[DenoiseGroup],
            filmFilteredBuffer.gpuMat,
            stream
        );
    }

    if (rgbBufferCounts[DenoiseGroup] > 0) {
        auto &cfg = statTypeConfigs[Radiance];
        auto &cudaGroups = cfg.cudaGroups;
        cv::cuda::stat_denoiser::filter<float3>(
            rgbBufferCounts[DenoiseGroup],
            width,
            height,
            filterDSFactor,
            filterRadius,
            denoiseFilm,
            nRGBGPUPtrs[DenoiseGroup],
            meanRGBGPUPtrs[DenoiseGroup],
            m2RGBGPUPtrs[DenoiseGroup],
            m3RGBGPUPtrs[DenoiseGroup],
            filmRGBGPUPtrs[DenoiseGroup],
            filmBuffer.gpuMat,
            gBufferGPUPtrs,
            gBufferChannelCountsGPUMat,
            gBufferDRFactorsGPUMat,
            gBuffers.size(),
            meanCorrRGBGPUPtrs[DenoiseGroup],
            discriminatorRGBGPUPtrs[DenoiseGroup],
            filmFilteredRGBGPUPtrs[DenoiseGroup],
            filmFilteredBuffer.gpuMat,
            stream
        );
    }
}

void Estimator::CalculateMeanVars() {
    #if DEBUG
        std::cout << "CalculateMeanVars()" << std::endl;
        std::cout << "  Float buffer count: " << (int) floatBufferCounts[CalculateMeanVarianceGroup] << std::endl;
        std::cout << "  RGB buffer count:   " << (int) rgbBufferCounts[CalculateMeanVarianceGroup] << std::endl;
    #endif

    // GPU variant, which is slower.
    // To be fair toward ProDen, the only technique that requires these estimates, we use the faster CPU code below.
    // if (floatBufferCounts[CalculateMeanVarianceGroup] > 0) {
    //     cv::cuda::stat_denoiser::calculateMeanVars<float>(
    //         floatBufferCounts[CalculateMeanVarianceGroup],
    //         width,
    //         height,
    //         nFGPUPtrs[CalculateMeanVarianceGroup],
    //         filmM2FGPUPtrs[CalculateMeanVarianceGroup],
    //         filmVarFGPUPtrs[CalculateMeanVarianceGroup],
    //         stream
    //     );
    // }

    // if (rgbBufferCounts[CalculateMeanVarianceGroup] > 0) {
    //     cv::cuda::stat_denoiser::calculateMeanVars<float3>(
    //         rgbBufferCounts[CalculateMeanVarianceGroup],
    //         width,
    //         height,
    //         nRGBGPUPtrs[CalculateMeanVarianceGroup],
    //         filmM2RGBGPUPtrs[CalculateMeanVarianceGroup],
    //         filmVarRGBGPUPtrs[CalculateMeanVarianceGroup],
    //         stream
    //     );
    // }

    auto &cfgs = statTypeConfigs;

    for (unsigned char i = 0; i < cfgs.nEnabled; i++) {
        auto &cfg = cfgs.configs[i];

        for (unsigned char j = cfg.bounceStart; j < cfg.bounceEnd; j++) {
            if (cfg.nChannels == 3) {
                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), CalculateMeanVarianceGroup) != cfg.cudaGroups.end() && j == 0) {
                    auto n = nBuffers[i][j].mat;
                    auto m2 = filmM2Buffers[i][j].mat;
                    auto var = filmVarBuffers[i][j].mat;

                    for(int row = 0; row < m2.rows; ++row) {
                        int *nP = n.ptr<int>(row);
                        Vec3f *m2P = m2.ptr<Vec3f>(row);
                        Vec3f *varP = var.ptr<Vec3f>(row);
                        float nPF = (float) *nP;

                        for(int col = 0; col < m2.cols; ++col) {
                            *varP++ = *m2P++ / ((nPF - 1.f) * nPF);
                            nP++;
                        }
                    }
                }
            } else {
                if (std::find(cfg.cudaGroups.begin(), cfg.cudaGroups.end(), CalculateMeanVarianceGroup) != cfg.cudaGroups.end() && j == 0) {
                    auto n = nBuffers[i][j].mat;
                    auto m2 = filmM2Buffers[i][j].mat;
                    auto var = filmVarBuffers[i][j].mat;

                    for(int row = 0; row < m2.rows; ++row) {
                        int *nP = n.ptr<int>(row);
                        float *m2P = m2.ptr<float>(row);
                        float *varP = var.ptr<float>(row);
                        float nPF = (float) *nP;

                        for(int col = 0; col < m2.cols; ++col) {
                            *varP++ = *m2P++ / ((nPF - 1.f) * nPF);
                            nP++;
                        }
                    }
                }
            }
        }
    }
}

void Estimator::Synchronize() {
    cv::cuda::stat_denoiser::synchronize(stream);
}

}  // namespace pbrt
