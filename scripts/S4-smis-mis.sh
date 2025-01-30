#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

mkdir -p out

# Activate standard MIS configuration
cp scenes/mis.pbrt scenes/_active.pbrt

cd out/
../build/pbrt-v3/pbrt --writeimages ../scenes/veach-mis/scene-stat.pbrt
cd ../
