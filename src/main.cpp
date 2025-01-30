#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "main.hpp"

int main(int argc, char** argv) {
    App* app = new App();
    return app->run();
}
