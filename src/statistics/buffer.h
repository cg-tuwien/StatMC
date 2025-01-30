// © 2024-2025 Hiroyuki Sakai

// We support only 32-bit floating-point output (due to limitations of OpenCV's imwrite() and pbrt-v4's DisplayStatic() methods).

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_STATISTICS_BUFFER_H
#define PBRT_STATISTICS_BUFFER_H

#include "pbrt.h"
#include "statistics/statpbrt.h"
#include <regex>

namespace pbrt {

class Buffer {
    public:
        Buffer() {}
        Buffer(
            const std::string &name,
            Mat mat
        ) : Buffer(name, mat, GpuMat(mat.rows, mat.cols, mat.type()))
        {}
        Buffer(
            const std::string &name,
            Mat mat,
            GpuMat gpuMat
        ) : name(name),
            mat(mat),
            matPtr(mat.ptr()),
            gpuMat(gpuMat)
        {
            const unsigned char nChannels = mat.channels();
            switch (nChannels) {
                case 1:
                    channelNames = {name};
                    break;
                case 3:
                    channelNames = {name + ".R", name + ".G", name + ".B"};
                    break;
                default:
                    channelNames = std::vector<std::string>(nChannels);
                    unsigned char i = 1;
                    std::generate(channelNames.begin(), channelNames.end(), [&name, &i] { return name + "." + std::to_string(i++); });
                    break;
            }

            if (mat.depth() == CV_32F)
                outMat = mat;
            else
                mat.convertTo(outMat, CV_32F); // Preallocate outMat so that it is always available
        }

        inline void upload(cv::cuda::Stream &stream) {
            gpuMat.upload(mat, stream);
        }

        inline void download(cv::cuda::Stream &stream) {
            gpuMat.download(mat, stream);
        }

        const std::string name;
        std::vector<std::string> channelNames;
        Mat mat;
        const uchar *matPtr;
        GpuMat gpuMat;
        Mat outMat; // Matrix for output (CV_32F depth)
};

class BufferRegistry {
    public:
        BufferRegistry(const Buffer &filmBuffer) {
            buffers.push_back(filmBuffer);
        };
        void Register(const Buffer &b);
        std::vector<Buffer> buffers;
};

class OutputBufferSelection {
    public:
        OutputBufferSelection(
            const BufferRegistry &reg,
            const std::string &filename
        );
        OutputBufferSelection(
            const BufferRegistry &reg,
            const std::regex &regex,
            const std::string &filename
        );
        void PrepareOutput() const;
        void Write(const std::string &filenameSuffix = "") const;
        void Display(const std::string &titleSuffix = "") const;
        std::string GetFilenameStem() const;

    private:
        void SetFilename(const std::string &filename);
        void Append(const Buffer &b);

        std::vector<Buffer> buffers;
        std::vector<Mat> outMats;
        std::string filenameStem;
        std::string filenameExtension;
        std::vector<std::string> channelNames;
};

}  // namespace pbrt

#endif  // PBRT_STATISTICS_BUFFER_H
