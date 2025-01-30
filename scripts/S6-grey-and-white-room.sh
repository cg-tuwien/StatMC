#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

mkdir -p out

# Activate rendering and denoising configuration
cp scenes/render-denoise.pbrt scenes/_active.pbrt

cd out/
../build/pbrt-v3/pbrt --writeimages ../scenes/living-room/scene-stat.pbrt
cd ../
