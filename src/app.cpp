#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "main.hpp"

App::App() {
    // setup sdl2 context
    // returns 0 on success, so should fail
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        printf("FATAL ERROR: UNABLE TO INITALIZE SDL2\n");
        printf("ERROR MESSAGE: %s\n", SDL_GetError());
        return;
    }

    this->window = SDL_CreateWindow("IMGUI TESTING", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (this->window == NULL) {
        printf("FATAL ERROR: UNABLE TO CREATE WINDOW CONTEXT\n");
        printf("ERROR MESSAGE: %s\n", SDL_GetError());
        return;
    }

    this->renderer = SDL_CreateRenderer(window,  -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (this->renderer == NULL) {
        printf("FATAL ERROR: UNABLE TO CREATE RENDERING CONTEXT\n");
        printf("ERROR MESSAGE: %s\n", SDL_GetError());
        return;
    }

    // imgui backend setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // IMGUI implementation setup
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // setup class variables
    this->closed = false;
    this->plots = std::vector<Plot*>();
    this->plots.push_back(new Plot(10, 10, 150, COLOR_GREEN));
    this->plots.push_back(new Plot(200, 10, 150, COLOR_ORANGE));
    this->selectedPlot = nullptr;

    // load assets
    this->loadAssets();
}

App::~App() {
    // shutdown IMGUI
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // shutdown SDL2
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int App::run() {
    while (!this->closed) {
        this->update();
        this->updateCursor();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        this->renderEngine();

        this->renderGUI();
        ImGui::Render();
        
        //ImGuiIO& io = ImGui::GetIO();
        //SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    return 0;
}

void App::update() {
    SDL_Event event;
    this->selectedPlot = nullptr;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            this->closed = true;
        }
    }

    SDL_Point lastmouse = this->mouse;
    SDL_GetMouseState(&this->mouse.x, &this->mouse.y);
    this->deltaMouse.x = this->mouse.x - lastmouse.x;
    this->deltaMouse.y = this->mouse.y - lastmouse.y;
    this->selectedPlot = nullptr;

    for (Plot* plot : this->plots) {
        plot->update(&this->mouse, &this->deltaMouse, this->plots);

        if ((int) plot->mouseState > (int) MouseState::OUT_OF_BOUNDS) {
            this->selectedPlot = plot;
        }
    }

    this->updateCursor();
}

void App::updateCursor() {
    if (this->selectedPlot == nullptr) {
        SDL_SetCursor(this->arrowCursor);
        return;
    }

    switch (this->selectedPlot->mouseState) {
        case MouseState::HOVERED_CENTER: 
            SDL_SetCursor(this->handCursor);
            break;
        case MouseState::SELECTED_CENTER:
            SDL_SetCursor(this->arrowCursor);
            break;
        case MouseState::HOVERED_LEFT:
        case MouseState::HOVERED_RIGHT:
        case MouseState::SELECTED_LEFT:
        case MouseState::SELECTED_RIGHT:
            SDL_SetCursor(this->resizeHCursor);
            break;
        case MouseState::HOVERED_TOP:
        case MouseState::HOVERED_BOTTOM:
        case MouseState::SELECTED_TOP:
        case MouseState::SELECTED_BOTTOM:
            SDL_SetCursor(this->resizeVCursor);
            break;
        default:
            SDL_SetCursor(this->arrowCursor);
    }
}

void App::renderEngine() {
    for (auto plot : this->plots) {
        plot->render(this->renderer);
    }

    // print out plot 0's mouse state for debug purposes
    //printf("plot 1: %s\n", this->plots.at(0)->stateAsString());
}

void App::renderGUI() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    int id = 0;

    for (auto plot : this->plots) {
        if (plot->renderTooltip) {
            ImGui::SetNextWindowPos(ImVec2(this->mouse.x + 16, this->mouse.y + 16));
            ImGui::SetTooltip("BITCHASS LMAO\n\nPlot %d: %s\nSize: %dx%d\nOffset: (%d, %d)", id, plot->crop.c_str(), plot->bounds.w, plot->bounds.h, plot->bounds.x, plot->bounds.y);
        }

        id++;
    }
}

void App::loadAssets() {
    this->handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    this->arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    this->resizeVCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    this->resizeHCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
}