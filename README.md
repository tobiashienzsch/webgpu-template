# WebGPU Compute Template

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE.txt)
[![Build](https://github.com/tobiashienzsch/webgpu-template/actions/workflows/build.yml/badge.svg)](https://github.com/tobiashienzsch/webgpu-template/actions/workflows/build.yml)

## Building for Emscripten:

1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
2. Install Ninja build system
3. `emcmake cmake -S .  -B build -G "Ninja Multi-Config"`
4. `cmake --build build --config Debug`
5. `emrun build/Debug/index.html`
