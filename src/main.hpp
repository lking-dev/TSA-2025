#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

#include <vector>
#include <iostream>
#include <string>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 640

#define PLOT_LINE_SPACING 10
#define PLOT_PADDING (PLOT_LINE_SPACING / 2)
#define PLOT_MIN_WIDTH 64
#define PLOT_MIN_HEIGHT 64

#define COLOR_GREEN (SDL_Color) {0x10, 0xD0, 0x10, 0xFF}
#define COLOR_ORANGE (SDL_Color) {0xED, 0x91, 0x21, 0xFF}

#define RESIZE_TOLERANCE 10

class App;
struct Plot;

enum class MouseState {
    HOVERED_TOP,
    HOVERED_RIGHT,
    HOVERED_BOTTOM,
    HOVERED_LEFT,
    HOVERED_CENTER,

    SELECTED_TOP,
    SELECTED_RIGHT,
    SELECTED_BOTTOM,
    SELECTED_LEFT,
    SELECTED_CENTER,
    
    OUT_OF_BOUNDS = -1
};

class App {
private:
    // SDL2 related
    SDL_Window* window;
    SDL_Renderer* renderer;

    // IMGUI related

    // app variables
    bool closed;
    std::vector<Plot*> plots;
    Plot* selectedPlot;
    SDL_Point mouse;
    SDL_Point deltaMouse;

    // assets
    SDL_Cursor* handCursor;
    SDL_Cursor* arrowCursor;
    SDL_Cursor* resizeVCursor;
    SDL_Cursor* resizeHCursor;

public:
    // constructor, takes care of initializing SDL2 and IMGUI
    App();
    // destructor, destroys window and closes SDL2 and IMGUI contexts
    ~App();

public:
    // application mainloop
    int run();

private:
    void update();
    void updateCursor();
    void renderEngine();
    void renderGUI();

private:
    void loadAssets();
};

struct Plot {
    // sdl properties
    SDL_Rect bounds;
    SDL_Color color;
    SDL_Rect tmp;

    // mouse data
    MouseState mouseState;
    int currentMouse;
    int previousMouse;

    // copy of main app's information
    SDL_Point mouse;
    SDL_Point deltaMouse;

    // plot data
    bool renderTooltip;
    std::string crop;

    Plot(int x, int y, int size, SDL_Color color);

    void update(SDL_Point* mouse, SDL_Point* deltaMouse, std::vector<Plot*>& list);
    MouseState updateMouseState();
    void updatePosition(std::vector<Plot*>& plots);
    void render(SDL_Renderer* renderer);

    void move(int deltaX, int deltaY);
    void resize(int deltaWidth, int deltaHeight);
    bool checkCollisions(Plot* plot, std::vector<Plot*>& list);

    const char* stateAsString();
};