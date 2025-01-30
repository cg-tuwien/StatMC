#!/bin/bash

# © 2024-2025 Hiroyuki Sakai

PBRT_BUILD_PATH="../../../../build/pbrt-v3/"
N_SAMPLES=32768

${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material UberMaterial > uber.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material GlassMaterial > glass.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material HairMaterial > hair.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material MatteMaterial > matte.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material MetalMaterial > metal.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material PlasticMaterial > plastic.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material SubstrateMaterial > substrate.json
${PBRT_BUILD_PATH}/precomputealbedo --nsamples $N_SAMPLES --material TranslucentMaterial > translucent.json

./json2cpp.sh uber.json
./json2cpp.sh glass.json
./json2cpp.sh hair.json
./json2cpp.sh matte.json
./json2cpp.sh metal.json
./json2cpp.sh plastic.json
./json2cpp.sh substrate.json
./json2cpp.sh translucent.json
