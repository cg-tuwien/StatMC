// © 2024-2025 Hiroyuki Sakai

#include "statistics/buffer.h"
#include "pbrt/util/display.h"

namespace pbrt {

void BufferRegistry::Register(const Buffer &b) {
    buffers.push_back(b);
}

OutputBufferSelection::OutputBufferSelection(
    const BufferRegistry &reg,
    const std::string &filename
) {
    SetFilename(filename);

    for (const Buffer &b : reg.buffers)
        Append(b);
}

OutputBufferSelection::OutputBufferSelection(
    const BufferRegistry &reg,
    const std::regex &regex,
    const std::string &filename
) {
    SetFilename(filename);

    for (const Buffer &b : reg.buffers)
        if (std::regex_match(b.name, regex))
            Append(b);
}

void OutputBufferSelection::PrepareOutput() const {
    for (const Buffer &b : buffers)
        if (b.mat.depth() != CV_32F)
            b.mat.convertTo(b.outMat, CV_32F);
}

void OutputBufferSelection::Write(const std::string &filenameSuffix) const {
    for (const Buffer &b : buffers) {
        std::string filename =
            (filenameSuffix.empty() ? filenameStem : filenameStem + "-" + filenameSuffix) +
            "-" + b.name + "." + filenameExtension;

        if (b.outMat.channels() == 3) {
            Mat outMat;
            cv::cvtColor(b.outMat, outMat, cv::COLOR_RGB2BGR);
            cv::imwrite(filename, outMat);
        } else
            cv::imwrite(filename, b.outMat);
    }
}

static PBRT_CONSTEXPR char MaxNDisplayBuffers = 100; // We get a segfault for higher values.
void OutputBufferSelection::Display(const std::string &titleSuffix) const {
    if (outMats.size() > 0) {
        if (outMats.size() > MaxNDisplayBuffers)
             LOG(FATAL) << "Exceeded the maximum number of buffers for display!";

        Mat outMat;
        cv::merge(outMats, outMat);
        pbrtv4::DisplayStatic(
            titleSuffix.empty() ? filenameStem : filenameStem + "-" + titleSuffix,
            outMat.cols,
            outMat.rows,
            outMat.ptr<float>(),
            channelNames
        );
    }
}

std::string OutputBufferSelection::GetFilenameStem() const {
    return filenameStem;
}

void OutputBufferSelection::SetFilename(const std::string &filename) {
    const size_t pos = filename.find_last_of(".");
    filenameStem = filename.substr(0, pos);
    filenameExtension = filename.substr(pos + 1);
}

void OutputBufferSelection::Append(const Buffer &b) {
    buffers.push_back(b);
    outMats.push_back(b.outMat);
    channelNames.insert(channelNames.end(), b.channelNames.begin(), b.channelNames.end());
}

}  // namespace pbrt
