#include "AudioDevice.hpp"
#include "Window.hpp"

#include <cstdlib>

int main(int, char**) {
    auto audioDevice = tobi::AudioDevice{};
    auto window = tobi::Window{};
    window.show();
    return EXIT_SUCCESS;
}
