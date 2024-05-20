#include "AudioDevice.hpp"

#include <fmt/format.h>
#include <fmt/os.h>

#include <cassert>
#include <stdexcept>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace tobi {

AudioDevice::~AudioDevice() {
    if (ma_device_get_state(&_device) != ma_device_state_uninitialized) {
        /* Uninitialize the waveform after the device so we don't pull it from under the device
         * while it's being reference in the data callback. */
        ma_device_uninit(&_device);
        ma_waveform_uninit(&_sineWave);
    }
}

auto AudioDevice::initialized() -> void {
    if (not isInitialized()) {
        auto config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = audioDeviceFormat;
        config.playback.channels = audioDeviceChannels;
        config.sampleRate = audioDeviceSamplerate;
        config.pUserData = &_sineWave;
        config.dataCallback = [](ma_device* device, void* output, const void* /*input*/,
                                 ma_uint32 frames) -> void {
            assert(device->playback.channels == audioDeviceChannels);
            assert(device->playback.format == audioDeviceFormat);

            auto* sineWave = static_cast<ma_waveform*>(device->pUserData);
            assert(sineWave != nullptr);

            ma_waveform_read_pcm_frames(sineWave, output, frames, nullptr);
        };

        if (ma_device_init(nullptr, &config, &_device) != MA_SUCCESS) {
            throw std::runtime_error("Failed to open playback device");
        }

        fmt::println("Device Name: {}", _device.playback.name);

        auto wave = ma_waveform_config_init(_device.playback.format, _device.playback.channels,
                                            _device.sampleRate, ma_waveform_type_sine, 0.2, 440.0);
        ma_waveform_init(&wave, &_sineWave);

        if (ma_device_start(&_device) != MA_SUCCESS) {
            throw std::runtime_error("Failed to start playback device");
        }
    }
}

auto AudioDevice::isInitialized() const -> bool {
    return ma_device_is_started(&_device);
}

}  // namespace tobi
