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

#ifndef PBRT_UTIL_FILE_H
#define PBRT_UTIL_FILE_H

#include <pbrt/pbrt.h>

#include <pbrt/util/pstd.h>

#include <string>
#include <vector>

namespace pbrtv4 {

// File and Filename Function Declarations
std::string ReadFileContents(std::string filename);
// std::string ReadDecompressedFileContents(std::string filename);
bool WriteFileContents(std::string filename, const std::string &contents);

std::vector<Float> ReadFloatFile(std::string filename);

bool FileExists(std::string filename);
bool RemoveFile(std::string filename);

std::string ResolveFilename(std::string filename);
void SetSearchDirectory(std::string filename);

bool HasExtension(std::string filename, std::string ext);
std::string RemoveExtension(std::string filename);

std::vector<std::string> MatchingFilenames(std::string filename);

FILE *FOpenRead(std::string filename);
FILE *FOpenWrite(std::string filename);

}  // namespace pbrtv4

#endif  // PBRT_UTIL_FILE_H
