// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

/*
    This file contains modifications to the original pbrt source code for the
    paper "A Statistical Approach to Monte Carlo Denoising"
    (https://www.cg.tuwien.ac.at/StatMC).
    
    Copyright © 2024-2025 Hiroyuki Sakai for the modifications.
    Original copyright and license (refer to the top of the file) remain
    unaffected.
 */

#ifndef PBRT_UTIL_LOG_H
#define PBRT_UTIL_LOG_H

#include <pbrt/pbrt.h>

#include <string>
#include <vector>

namespace pbrtv4 {

// LogLevel Definition
enum class LogLevel { Verbose, Error, Fatal, Invalid };

std::string ToString(LogLevel level);
LogLevel LogLevelFromString(const std::string &s);

void ShutdownLogging();
void InitLogging(LogLevel level, std::string logFile, bool logUtilization, bool useGPU);

#ifdef PBRT_BUILD_GPU_RENDERER

struct GPULogItem {
    LogLevel level;
    char file[64];
    int line;
    char message[128];
};

std::vector<GPULogItem> ReadGPULogs();

#endif

// LogLevel Global Variable Declaration
namespace logging {
extern LogLevel logLevel;
extern FILE *logFile;
}  // namespace logging

// Logging Function Declarations
PBRT_CPU_GPU
void Log(LogLevel level, const char *file, int line, const char *s);

PBRT_CPU_GPU [[noreturn]] void LogFatal(LogLevel level, const char *file, int line,
                                        const char *s);

template <typename... Args>
PBRT_CPU_GPU inline void Log(LogLevel level, const char *file, int line, const char *fmt,
                             Args &&...args);

template <typename... Args>
PBRT_CPU_GPU [[noreturn]] inline void LogFatal(LogLevel level, const char *file, int line,
                                               const char *fmt, Args &&...args);

#define TO_STRING(x) TO_STRING2(x)
#define TO_STRING2(x) #x

#ifdef PBRT_IS_GPU_CODE

extern __constant__ LogLevel LOGGING_LogLevelGPU;

#define LOG_VERBOSE(...)                               \
    (pbrtv4::LogLevel::Verbose >= LOGGING_LogLevelGPU && \
     (pbrtv4::Log(LogLevel::Verbose, __FILE__, __LINE__, __VA_ARGS__), true))

#define LOG_ERROR(...)                               \
    (pbrtv4::LogLevel::Error >= LOGGING_LogLevelGPU && \
     (pbrtv4::Log(LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__), true))

#define LOG_FATAL(...) \
    pbrtv4::LogFatal(pbrtv4::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)

#else

// Logging Macros
#define LOG_VERBOSE(...)                             \
    (pbrtv4::LogLevel::Verbose >= logging::logLevel && \
     (pbrtv4::Log(LogLevel::Verbose, __FILE__, __LINE__, __VA_ARGS__), true))

#define LOG_ERROR(...)                                   \
    (pbrtv4::LogLevel::Error >= pbrtv4::logging::logLevel && \
     (pbrtv4::Log(LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__), true))

#define LOG_FATAL(...) \
    pbrtv4::LogFatal(pbrtv4::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)

#endif

}  // namespace pbrtv4

#include <pbrt/util/print.h>

namespace pbrtv4 {

template <typename... Args>
inline void Log(LogLevel level, const char *file, int line, const char *fmt,
                Args &&...args) {
#ifdef PBRT_IS_GPU_CODE
    Log(level, file, line, fmt);  // just the format string #yolo
#else
    std::string s = StringPrintf(fmt, std::forward<Args>(args)...);
    Log(level, file, line, s.c_str());
#endif
}

template <typename... Args>
inline void LogFatal(LogLevel level, const char *file, int line, const char *fmt,
                     Args &&...args) {
#ifdef PBRT_IS_GPU_CODE
    LogFatal(level, file, line, fmt);  // just the format string #yolo
#else
    std::string s = StringPrintf(fmt, std::forward<Args>(args)...);
    LogFatal(level, file, line, s.c_str());
#endif
}

}  // namespace pbrtv4

#endif  // PBRT_UTIL_LOG_H
