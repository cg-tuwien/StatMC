#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

mkdir -p out

# Activate rendering and denoising configuration
cp scenes/render-denoise.pbrt scenes/_active.pbrt

cd out/
../build/pbrt-v3/pbrt --writeimages --baseseed 1 ../scenes/staircase/scene-stat.pbrt
cd ../
