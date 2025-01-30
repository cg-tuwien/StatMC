#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

mkdir -p out

# Activate standard Russian roulette configuration
cp scenes/rr.pbrt scenes/_active.pbrt

cd out/
../build/pbrt-v3/pbrt --writeimages ../scenes/veach-ajar/scene-stat.pbrt
cd ../
