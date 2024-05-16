add_library(miniaudio)
target_sources(miniaudio PRIVATE miniaudio.cpp)
target_include_directories(miniaudio PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/miniaudio)