#include <tobi/AudioDevice.hpp>
#include <tobi/Window.hpp>

#include <clap/clap.h>

#include <cstdlib>

int main(int, char**) {
    auto audioDevice = tobi::AudioDevice{};
    auto window = tobi::Window{};
    window.show();
    return EXIT_SUCCESS;
}
