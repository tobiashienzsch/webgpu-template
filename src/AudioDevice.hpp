#pragma once

#include "miniaudio.h"

namespace tobi {

struct AudioDevice {
    AudioDevice() = default;
    ~AudioDevice();

    AudioDevice(AudioDevice const& other) = delete;
    AudioDevice(AudioDevice&& other) = delete;

    auto operator=(AudioDevice const& other) -> AudioDevice& = delete;
    auto operator=(AudioDevice&& other) -> AudioDevice& = delete;

    auto initialized() -> void;

    [[nodiscard]] auto isInitialized() const -> bool;

  private:
    static constexpr auto audioDeviceFormat = ma_format_f32;
    static constexpr auto audioDeviceChannels = 2;
    static constexpr auto audioDeviceSamplerate = 48000;

    ma_waveform _sineWave{};
    ma_device _device{};
};

}  // namespace tobi
