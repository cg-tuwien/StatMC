// © 2024-2025 Hiroyuki Sakai

#include <float.h>  // LDBL_DIG
#include <chrono>   // std::chrono
#include <iomanip>  // std::setprecision
#include <iostream> // std::cout, std::endl
#include <string>   // std::string
#include <thread>   // std::thread

#include "pbrt.h"
#include "geometry.h" // Vector, ...
#include "memory.h"   // MemoryArena
#include "parallel.h" // NumSystemCores()

#include "materials/disney.h"
#include "materials/fourier.h"
#include "materials/glass.h"
#include "materials/hair.h"
#include "materials/kdsubsurface.h"
#include "materials/matte.h"
#include "materials/metal.h"
#include "materials/mirror.h"
#include "materials/mixmat.h"
#include "materials/plastic.h"
#include "materials/substrate.h"
#include "materials/subsurface.h"
#include "materials/translucent.h"
#include "materials/uber.h"
#include "samplers/random.h"   // RNG
#include "textures/constant.h" // ConstantTexture

#if defined LDBL_DECIMAL_DIG
    #define OP_LDBL_DECIMAL_DIG (LDBL_DECIMAL_DIG)
#elif defined DECIMAL_DIG
    #define OP_LDBL_DECIMAL_DIG (DECIMAL_DIG)
#else  
    #define OP_LDBL_DECIMAL_DIG (LDBL_DIG + 3)
#endif

using namespace pbrt;

typedef long double MyFloat;
typedef unsigned long long MyCount;

static PBRT_CONSTEXPR long long DefaultNSamplesPerThread = 10000l;
static PBRT_CONSTEXPR long long DefaultBenchmarkNSamples = 100000000l;

static PBRT_CONSTEXPR char         LutWidth           = 8;
static PBRT_CONSTEXPR long double  LutCheckThreshold  = 0.05l;
static PBRT_CONSTEXPR unsigned int PbrtCheckNSamples  = 512;
static PBRT_CONSTEXPR long double  PbrtCheckThreshold = 0.05l;

static PBRT_CONSTEXPR char JsonInd1[] = "  ";
static PBRT_CONSTEXPR char JsonInd2[] = "    ";
static PBRT_CONSTEXPR char JsonInd3[] = "      ";
static PBRT_CONSTEXPR char ProgressBarWidth   = 10;

class Indexer {
    public:
        Indexer(const unsigned char nDims, const bool testLUT) : 
            nDims(nDims),
            testLUT(testLUT),
            rng(RNG())
        {
            indices    = new unsigned char[nDims];
            lengths    = new unsigned char[nDims];
            maxIndices = new unsigned char[nDims];

            for (unsigned char i = 0; i < nDims; i++) {
                indices[i] = 0;
                lengths[i] = LutWidth;
                maxIndices[i] = lengths[i] - 1;
            }

// For setting the lengths individually
#if 0
            const std::string materialName = "";
            if (materialName == "GlassMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kr [0,1]
                lengths[2] = 8; // Kt [0,1]
                lengths[3] = 8; // uRoughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[4] = 8; // vRoughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[5] = 8; // index [1+Epsilon,2.42]
            } else if (materialName == "HairMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // sigma_a [0,1]
                lengths[2] = 8; // beta_m [0,1]
                lengths[3] = 8; // beta_n [0,1]
            } else if (materialName == "MatteMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // sigma [0,90]
            } else if (materialName == "MetalMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // eta [Epsilon,7.14]
                lengths[2] = 8; // k [Epsilon,8.62]
                lengths[3] = 8; // uRoughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[4] = 8; // vRoughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
            } else if (materialName == "MirrorMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kr [0,1]
            } else if (materialName == "PlasticMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kd [0,1]
                lengths[2] = 8; // Ks [0,1]
                lengths[3] = 8; // roughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
            } else if (materialName == "SubstrateMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kd [0,1]
                lengths[2] = 8; // Ks [0,1]
                lengths[3] = 8; // nu [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[4] = 8; // nv [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
            } else if (materialName == "TranslucentMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kd [0,1]
                lengths[2] = 8; // Ks [0,1]
                lengths[3] = 8; // roughness [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[4] = 8; // reflect [0,1]
                lengths[5] = 8; // transmit [0,1]
            } else if (materialName == "UberMaterial") {
                lengths[0] = 8; // wo [CosEpsilon,1]
                lengths[1] = 8; // Kd [0,1]
                lengths[2] = 8; // Ks [0,1]
                lengths[3] = 8; // Kr [0,1]
                lengths[4] = 8; // Kt [0,1]
                lengths[5] = 8; // roughnessu [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[6] = 8; // roughnessv [TrowbridgeAlphaMin,TrowbridgeAlphaMax]
                lengths[7] = 8; // eta [1+Epsilon/2.42]
            }
#endif
        }
        ~Indexer() {
            delete indices;
            delete lengths;
            delete maxIndices;
        }
        MyFloat GetFloat(const unsigned char i, Float v1 = 0., Float v2 = 1.) {
            Float t = testLUT ? rng.UniformFloat() : (Float)indices[i] / maxIndices[i];
            return pbrt::Lerp(t, v1, v2);
        }
        Spectrum GetSpectrum(const unsigned char i, Float v1 = 0., Float v2 = 1.) {
            Spectrum t = testLUT ? RandomSpectrum() : Spectrum((MyFloat)indices[i] / maxIndices[i]);
            return Lerp(t, v1, v2);
        }
        bool Increment() {
            indices[0]++;

            while (indices[dimIndex] == lengths[dimIndex]) {
                if (dimIndex == nDims - 1)
                    return false;
            
                indices[dimIndex++] = 0;
                indices[dimIndex]++;
            }

            dimIndex = 0;

            return true;
        }

        const unsigned char nDims;

        unsigned char *indices    = nullptr;
        unsigned char *lengths    = nullptr;
        unsigned char *maxIndices = nullptr;

    private:
        Spectrum RandomSpectrum() {
            Float rgb[3];
            for (unsigned char i = 0; i < 3; i++)
                rgb[i] = rng.UniformFloat();
            return Spectrum::FromRGB(rgb);
        }
        Spectrum Lerp(const Spectrum t, Float v1, Float v2) {
            Float rgb[3];
            t.ToRGB(rgb);
            for (unsigned char i = 0; i < 3; i++)
                rgb[i] = pbrt::Lerp(rgb[i], v1, v2);
            return Spectrum::FromRGB(rgb);
        }

        const bool testLUT;
        RNG rng;

        unsigned char dimIndex = 0;
};

void PrintHoursMinutesSeconds(const float seconds) {
    const unsigned short minutes = seconds / 60;
    const unsigned short hours = minutes / 60;

    std::clog << std::setw(2) << std::setfill('0') << hours << ":" <<
                 std::setw(2) << std::setfill('0') << minutes % 60 << ":" <<
                 std::setw(2) << std::setfill('0') << (unsigned short)seconds % 60;
}

void PrintProgress(
    const MyCount nTotal,
    const MyCount nProgress,
    const float duration
) {
    const float progress = (float)nProgress / nTotal;
    const float position = ProgressBarWidth * progress;
    const unsigned char intPosition = (unsigned char)position;
    const float fraction = position - intPosition;

    PrintHoursMinutesSeconds(duration);
    std::clog << "\u2595";
    for (unsigned char i = 0; i < ProgressBarWidth; i++) {
        if (i < intPosition)
            std::clog << "\u2588";
        else if (i == intPosition) {
            if      (fraction < .0625f) std::clog << " ";
            else if (fraction < .1875f) std::clog << "\u258F";
            else if (fraction < .3125f) std::clog << "\u258E";
            else if (fraction < .4375f) std::clog << "\u258D";
            else if (fraction < .5625f) std::clog << "\u258C";
            else if (fraction < .6875f) std::clog << "\u258B";
            else if (fraction < .8125f) std::clog << "\u258A";
            else if (fraction < .9375f) std::clog << "\u2589";
            else                        std::clog << "\u2588";
        }
        else
            std::clog << " ";
    }
    std::clog << "\u258F" << (int)(progress * 100.f) << " % ";
    PrintHoursMinutesSeconds((nTotal - nProgress) * duration / nProgress);
    std::clog << "\r";
    std::clog.flush();
}

void BenchmarkLUT(
    const Material *material,
    SurfaceInteraction *isect,
    const unsigned int nSamples,
    const bool full = false
) {
    std::clog << "Benchmarking " << nSamples << " LUT lookups..." << std::endl;
    Spectrum albedo;
    auto startTime = std::chrono::high_resolution_clock::now();
    for (unsigned int i = 0; i < nSamples; i++)
        // if (full)
        //     albedo += material->GetAlbedoFull(isect);
        // else
            albedo += material->GetAlbedo(isect);
    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - startTime;
    std::clog << "Result: " << albedo.y() << std::endl;
    std::clog << "Elapsed time: ";
    PrintHoursMinutesSeconds(duration.count());
    std::clog << std::endl;
}

void BenchmarkPBRT(
    const BSDF *bsdf,
    const Vector3f wo,
    const unsigned int nSamples
) {
    std::clog << "Benchmarking " << nSamples << " PBRT rho() calls..." << std::endl;

    RNG rng(0);
    Point2f rhoSamples[16];
    for (unsigned int i = 0; i < 16; i++)
        rhoSamples[i] = Point2f(rng.UniformFloat(), rng.UniformFloat());

    Spectrum albedo;
    auto startTime = std::chrono::high_resolution_clock::now();
    for (unsigned int i = 0; i < nSamples; i++)
        albedo += bsdf->rho(wo, 16, rhoSamples, BSDF_ALL);
    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - startTime;
    std::clog << "Result: " << albedo.y() << std::endl;
    std::clog << "Elapsed time: ";
    PrintHoursMinutesSeconds(duration.count());
    std::clog << std::endl;
}

const Vector3f normal(0.l, 0.l, 1.l);

void SampleAlbedo(
    const unsigned char threadId,
    const BSDF *bsdf,
    const Vector3f wo,
    const MyCount nSamples,
    MyFloat *sumF
) {
    RNG rng(threadId);

    Vector3f wi;
    BxDFType flags;
    *sumF = 0.l;
    for (MyCount i = 0; i < nSamples; i++) {
        Float pdf = 0.; // Use Float here, because the output from Sample_f is a Float
        const Spectrum f = bsdf->Sample_f(wo, &wi, {rng.UniformFloat(), rng.UniformFloat()}, &pdf, BSDF_ALL, &flags);
        Float fRGB[3];
        f.ToRGB(fRGB);
        if (pdf > 0.l && fRGB[0] > 0.l)
            *sumF += fRGB[0] * AbsCosTheta(wi) / pdf;
    }
}


int main(int argc, char *argv[]) {
    const auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << std::setprecision(OP_LDBL_DECIMAL_DIG); // Show enough digits for long double

    std::string materialName = "MatteMaterial";
    unsigned char nThreads = NumSystemCores();
    MyCount nSamples = DefaultNSamplesPerThread;
    MyCount seedOffset = 0;
    bool compareToPBRT = false;
    bool testLUT = false;
    bool benchmark = false;

    for (unsigned char i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--material"))
            materialName = argv[++i];
        if (!strcmp(argv[i], "--nthreads"))
            nThreads = atoi(argv[++i]);
        if (!strcmp(argv[i], "--seedoffset"))
            seedOffset = atoi(argv[++i]);
        if (!strcmp(argv[i], "--comparetopbrt")) // Compare calculated values to those from pbrt-v3's rho() function and emit warnings in case of significant differences
            compareToPBRT = true;
        if (!strcmp(argv[i], "--testlut")) // Randomize material parameters and compare calculated values to those from the LUT and emit warnings in case of significant differences
            testLUT = true;
        if (!strcmp(argv[i], "--benchmark")) // Do NOT calculate albedos and test performance of LUT lookups vs. pbrt rho() calls instead
            benchmark = true;
    }

    if (benchmark)
        nSamples = DefaultBenchmarkNSamples;

    for (unsigned char i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "--nsamples"))
            nSamples = atoi(argv[++i]);

    // Not implemented:
    // DisneyMaterial (too many parameters)
    // FourierMaterial (cannot be precomputed)
    // KdSubsurfaceMaterial (non-bounded parameter scale and mfp)
    // SubsurfaceMaterial (non-bounded parameter scale)
    if (materialName == "DisneyMaterial" ||
        materialName == "FourierMaterial" ||
        materialName == "KdSubsurfaceMaterial" ||
        materialName == "SubsurfaceMaterial")
        std::cerr << "Material \"" << materialName << "\" not supported." << std::endl;

    if (materialName != "GlassMaterial" &&
        materialName != "HairMaterial" &&
        materialName != "MatteMaterial" &&
        materialName != "MetalMaterial" &&
        materialName != "MirrorMaterial" &&
        materialName != "PlasticMaterial" &&
        materialName != "SubstrateMaterial" &&
        materialName != "TranslucentMaterial" &&
        materialName != "UberMaterial")
        materialName = "MatteMaterial";

    // Dynamically nested loop (http://stackoverflow.com/questions/18732974/c-dynamic-number-of-nested-for-loops-without-recursion)
    unsigned char nDims = 1;
    if      (materialName == "GlassMaterial")       nDims = 6;
    else if (materialName == "HairMaterial")        nDims = 4;
    else if (materialName == "MatteMaterial")       nDims = 2;
    else if (materialName == "MetalMaterial")       nDims = 5;
    else if (materialName == "MirrorMaterial")      nDims = 2;
    else if (materialName == "PlasticMaterial")     nDims = 4;
    else if (materialName == "SubstrateMaterial")   nDims = 5;
    else if (materialName == "TranslucentMaterial") nDims = 6;
    else if (materialName == "UberMaterial")        nDims = 8;
    Indexer ind(nDims, testLUT);

    if (!benchmark) {
        std::cout << "{" << std::endl;
        std::cout << JsonInd1 << "\"materialName\": \"" << materialName << "\"," << std::endl;
        std::cout << JsonInd1 << "\"nDims\": " << (unsigned short)ind.nDims << "," << std::endl;
        std::cout << JsonInd1 << "\"lengths\": [" << std::endl;
        for (unsigned char i = 0; i < ind.nDims; i++) {
            std::cout << JsonInd2 << (unsigned short)ind.lengths[i];
            if (i + 1 < ind.nDims)
                std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << JsonInd1 << "]," << std::endl;
        std::cout << JsonInd1 << "\"nThreads\": " << (unsigned short)nThreads << "," << std::endl;
        std::cout << JsonInd1 << "\"seedOffset\": " << (unsigned short)seedOffset << "," << std::endl;
        std::cout << JsonInd1 << "\"results\": [";
        std::cout << std::endl;
    }

    // Calculate total number of samples to report progress
    MyCount nIterations = 1;
    for (unsigned char i = 0; i < ind.nDims; i++)
        nIterations *= ind.lengths[i];
    const MyCount nSamplesTotal = nIterations * nSamples;
    MyCount nSamplesTotalProgress = 0;
    
    const auto loopStartTime = std::chrono::high_resolution_clock::now();
    while (true) {
        const MyFloat woz = ind.GetFloat(0, CosEpsilon, 1.l);
        Vector3f wo = Vector3f(std::sqrt(1.l - woz * woz), 0.l, woz); // We do not consider anisotropic materials
        wo = Normalize(wo);

        // Setup material
        Material *material = nullptr;
        if (materialName == "GlassMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kr      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Spectrum>> kt      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2));
            const std::shared_ptr<Texture<Float>> uRoughness = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (3, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> vRoughness = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (4, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> index      = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (5, 1.l + Epsilon,      2.42l));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new GlassMaterial(kr, kt, uRoughness, vRoughness, index, bumpMap, remapRoughness);
        } else if (materialName == "HairMaterial") {
            const std::shared_ptr<Texture<Spectrum>> sigmaA   = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1, Epsilon, 1.l));
            const std::shared_ptr<Texture<Spectrum>> color    = nullptr;
            const std::shared_ptr<Texture<Float>> eumelanin   = nullptr;
            const std::shared_ptr<Texture<Float>> pheomelanin = nullptr;
            const std::shared_ptr<Texture<Float>> eta         = std::make_shared<ConstantTexture<Float>>(1.55l);
            const std::shared_ptr<Texture<Float>> betaM       = std::make_shared<ConstantTexture<Float>>(ind.GetFloat(2, Epsilon, 1.l));
            const std::shared_ptr<Texture<Float>> betaN       = std::make_shared<ConstantTexture<Float>>(ind.GetFloat(3, Epsilon, 1.l));
            const std::shared_ptr<Texture<Float>> alpha       = std::make_shared<ConstantTexture<Float>>(2.l);

            material = new HairMaterial(sigmaA, color, eumelanin, pheomelanin, eta, betaM, betaN, alpha);
        } else if (materialName == "MatteMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kd   = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(1.l));
            const std::shared_ptr<Texture<Float>> sigma   = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat(1, 0.l, 90.l));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;

            material = new MatteMaterial(kd, sigma, bumpMap);
        } else if (materialName == "MetalMaterial") {
            const std::shared_ptr<Texture<Spectrum>> eta     = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1, Epsilon, 7.14l));
            const std::shared_ptr<Texture<Spectrum>> k       = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2, Epsilon, 8.62l));
            const std::shared_ptr<Texture<Float>> roughness  = nullptr;
            const std::shared_ptr<Texture<Float>> uRoughness = std::make_shared<ConstantTexture<Float>>(ind.GetFloat(3, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> vRoughness = std::make_shared<ConstantTexture<Float>>(ind.GetFloat(4, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new MetalMaterial(eta, k, roughness, uRoughness, vRoughness, bumpMap, remapRoughness);
        } else if (materialName == "MirrorMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kr = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;

            material = new MirrorMaterial(kr, bumpMap);
        } else if (materialName == "PlasticMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kd     = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Spectrum>> ks     = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2));
            const std::shared_ptr<Texture<Float>> roughness = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (3, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new PlasticMaterial(kd, ks, roughness, bumpMap, remapRoughness);
        } else if (materialName == "SubstrateMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kd = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Spectrum>> ks = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2));
            const std::shared_ptr<Texture<Float>> nu    = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (3, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> nv    = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (4, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new SubstrateMaterial(kd, ks, nu, nv, bumpMap, remapRoughness);
        } else if (materialName == "TranslucentMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kd       = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Spectrum>> ks       = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2));
            const std::shared_ptr<Texture<Float>> roughness   = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat   (3, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Spectrum>> reflect  = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(4));
            const std::shared_ptr<Texture<Spectrum>> transmit = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(5));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new TranslucentMaterial(kd, ks, roughness, reflect, transmit, bumpMap, remapRoughness);
        } else if (materialName == "UberMaterial") {
            const std::shared_ptr<Texture<Spectrum>> kd      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(1));
            const std::shared_ptr<Texture<Spectrum>> ks      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(2));
            const std::shared_ptr<Texture<Spectrum>> kr      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(3));
            const std::shared_ptr<Texture<Spectrum>> kt      = std::make_shared<ConstantTexture<Spectrum>>(ind.GetSpectrum(4));
            const std::shared_ptr<Texture<Float>> roughness  = nullptr;
            const std::shared_ptr<Texture<Float>> roughnessu = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat(5, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Float>> roughnessv = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat(6, TrowbridgeAlphaMin, TrowbridgeAlphaMax));
            const std::shared_ptr<Texture<Spectrum>> opacity = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(1.l));
            const std::shared_ptr<Texture<Float>> eta        = std::make_shared<ConstantTexture<Float>>   (ind.GetFloat(7, 1.l + Epsilon, 2.42l));
            const std::shared_ptr<Texture<Float>> bumpMap = nullptr;
            const bool remapRoughness = false;

            material = new UberMaterial(kd, ks, kr, kt, roughness, roughnessu, roughnessv, opacity, eta, bumpMap, remapRoughness);
        }

        // Setup BSDF
        SurfaceInteraction isect(Point3f (0.l, 0.l, 0.l), // p
                                 Vector3f(0.l, 0.l, 0.l), // pError
                                 Point2f (0.l, 0.l),      // uv
                                 wo,                      // wo
                                 Vector3f(1.l, 0.l, 0.l), // dpdu
                                 Vector3f(0.l, 1.l, 0.l), // dpdv
                                 Normal3f(0.l, 0.l, 0.l), // dndu
                                 Normal3f(0.l, 0.l, 0.l), // dndv
                                 MyFloat (0.l),           // time
                                 nullptr);                // shape
        MemoryArena arena;
        material->ComputeScatteringFunctions(&isect, arena, TransportMode::Radiance, false);
        const BSDF *bsdf = isect.bsdf;


        if (benchmark) {
            BenchmarkLUT(material, &isect, nSamples);
            BenchmarkPBRT(bsdf, wo, nSamples);
            delete material;
        } else {
            // Calculate albedo
            std::thread *threads = new std::thread[nThreads];
            MyFloat *sumFs = new MyFloat[nThreads];
            MyFloat sumF = 0.l;
            for (unsigned char i = 0; i < nThreads; i++)
                threads[i] = std::thread(SampleAlbedo, seedOffset + i,
                                         bsdf, wo,
                                         nSamples,
                                         &sumFs[i]);
            for (unsigned char i = 0; i < nThreads; i++) {
                threads[i].join();
                sumF += sumFs[i];
            }
            delete[] threads;
            delete[] sumFs;

            const MyCount nF = nThreads * nSamples;
            const MyFloat albedo = sumF / nF;


            // Checks
            if (albedo > 1.l)
                std::cerr << "Warning: calculated albedo " << albedo << " is greater than 1." << std::endl;

            Float lutAlbedoRGB[3];
            material->GetAlbedo(&isect).ToRGB(lutAlbedoRGB);
#ifdef PBRT_STATISTICS_FULL_LOOKUPS
            Float lutFullAlbedoRGB[3];
            material->GetAlbedoFull(&isect).ToRGB(lutFullAlbedoRGB);
#endif
            delete material;
            if (std::abs(albedo - lutAlbedoRGB[0]) > LutCheckThreshold)
                std::cerr << "Warning: calculated albedo " << albedo << " is significantly different from LUT albedo (" << lutAlbedoRGB[0] << ")." << std::endl;

            Float pbrtAlbedoRGB[3];
            if (compareToPBRT) {
                RNG rng(0);
                Point2f rhoSamples[PbrtCheckNSamples];
                for (unsigned int i = 0; i < PbrtCheckNSamples; i++)
                    rhoSamples[i] = Point2f(rng.UniformFloat(), rng.UniformFloat());

                const Spectrum pbrtAlbedo = bsdf->rho(wo, PbrtCheckNSamples, rhoSamples, BSDF_ALL);
                pbrtAlbedo.ToRGB(pbrtAlbedoRGB);

                if (std::abs(albedo - pbrtAlbedoRGB[0]) > PbrtCheckThreshold)
                    std::cerr << "Warning: calculated albedo " << albedo << " is significantly different from pbrt's rho (" << pbrtAlbedoRGB[0] << ")." << std::endl;
            }

#ifdef PBRT_STATISTICS_FULL_LOOKUPS
            if (std::abs(lutFullAlbedoRGB[0] - lutAlbedoRGB[0]) > LutCheckThreshold ||
                std::abs(lutFullAlbedoRGB[1] - lutAlbedoRGB[1]) > LutCheckThreshold ||
                std::abs(lutFullAlbedoRGB[2] - lutAlbedoRGB[2]) > LutCheckThreshold) {
                std::cerr << "Fatal: found a significant difference between reduced LUT and full LUT." << std::endl;
                exit(1);
            }
#endif


            // Output
            std::string indexString;
            for (unsigned char i = 0; i < ind.nDims; i++) {
                indexString += std::to_string(ind.GetFloat(i));
                if (i < ind.nDims - 1)
                    indexString += " ";
            }

            std::cout << JsonInd2 << "{" << std::endl;
            if (!testLUT)
                std::cout << JsonInd3 << "\"indices\": \"" << indexString << "\"," << std::endl;
            std::cout << JsonInd3 << "\"albedo\":             " << albedo << "," << std::endl;
            std::cout << JsonInd3 << "\"albedo (LUT)\":      \"" << lutAlbedoRGB[0]     << " " << lutAlbedoRGB[1]     << " " << lutAlbedoRGB[2]     << "\"," << std::endl;
#ifdef PBRT_STATISTICS_FULL_LOOKUPS
            std::cout << JsonInd3 << "\"albedo (full LUT)\": \"" << lutFullAlbedoRGB[0] << " " << lutFullAlbedoRGB[1] << " " << lutFullAlbedoRGB[2] << "\"," << std::endl;
#endif
            if (compareToPBRT)
                std::cout << JsonInd3 << "\"albedo (pbrt)\":     \"" << pbrtAlbedoRGB[0] << " " << pbrtAlbedoRGB[1] << " " << pbrtAlbedoRGB[2] << "\"," << std::endl;
            std::cout << JsonInd3 << "\"sumF\": " << sumF << "," << std::endl;
            std::cout << JsonInd3 << "\"nF\": " << nF << std::endl;
            std::cout << JsonInd2 << "}";

            nSamplesTotalProgress += nSamples;
            const std::chrono::duration<double> duration = (std::chrono::high_resolution_clock::now() - loopStartTime);
            PrintProgress(nSamplesTotal, nSamplesTotalProgress, duration.count());

            if (!ind.Increment()) {
                std::cout << std::endl;
                goto breakAll;
            }

            std::cout << "," << std::endl;
        }
    }

    breakAll:

    std::cout << JsonInd1 << "]" << std::endl;
    std::cout << "}" << std::endl;

    std::clog << std::endl;

    const std::chrono::duration<double> duration = (std::chrono::high_resolution_clock::now() - startTime);
    std::clog << "Elapsed time: ";
    PrintHoursMinutesSeconds(duration.count());
    std::clog << std::endl;

    return 0;
}
