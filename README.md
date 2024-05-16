# Building for desktop (WebGPU-native) with Dawn:
#  1. git clone https://github.com/google/dawn dawn
#  2. cmake -B build
#  3. cmake --build build
# The resulting binary will be found at one of the following locations:
#   * build/Debug/example[.exe]
#   * build/example[.exe]

# Building for Emscripten:
#  1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
#  2. Install Ninja build system
#  3. emcmake cmake -G Ninja -B build
#  3. cmake --build build
#  4. emrun build/index.html
