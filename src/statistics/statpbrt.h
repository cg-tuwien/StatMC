// © 2024-2025 Hiroyuki Sakai

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_STATISTICS_STATPBRT_H
#define PBRT_STATISTICS_STATPBRT_H

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/cudaimgproc.hpp>
#include "geometry.h"

namespace pbrt {

using cv::Vec3f;
using cv::Mat;
using cv::Mat_;
using cv::Mat1f;
using cv::Mat1i;
using cv::Mat3f;
using cv::cuda::GpuMat;

using Vec3  = cv::Vec<Float, 3>;
using Mat1  = Mat_<Float>;
using Mat3  = Mat_<Vec3>;

class Estimator;

}  // namespace pbrt

#endif  // PBRT_STATISTICS_STATPBRT_H
