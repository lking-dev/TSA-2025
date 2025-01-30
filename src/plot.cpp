#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "main.hpp"

Plot::Plot(int x, int y, int size, SDL_Color color) {
    this->bounds = (SDL_Rect){x, y, size, size};
    this->color = color;

    this->crop = std::string("Wheat");
}

void Plot::render(SDL_Renderer* renderer) {
    // draw the outline of the plot
    if (((int) this->mouseState >= 6) && ((int) this->mouseState <= 10)) {
        SDL_SetRenderDrawColor(renderer, 0xD0, 0xD0, 0xD0, 0xFF);
    } else if (((int) this->mouseState >= 1) && ((int) this->mouseState <= 5)) {
        SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF);
    } else {
        SDL_SetRenderDrawColor(renderer, 0x40, 0x40, 0x40, 0xFF);
    }

    SDL_RenderDrawRect(renderer, &this->bounds);

    // set rendering color to color of plot
    SDL_SetRenderDrawColor(renderer, this->color.r, this->color.g, this->color.b, this->color.a);
    // calculate rendering offsets for calcs
    int offx = this->bounds.x + PLOT_PADDING;
    int offy = this->bounds.y + PLOT_PADDING;
    int effectiveWidth = this->bounds.w - (PLOT_PADDING * 2);
    int effectiveHeight = this->bounds.h - (PLOT_PADDING * 2);
    int largestBound = effectiveWidth > effectiveHeight ? effectiveWidth : effectiveHeight;
    int partitions = largestBound / PLOT_LINE_SPACING;
    
    SDL_Texture* renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        effectiveWidth * 2, effectiveHeight * 2);
    SDL_SetRenderTarget(renderer, renderTexture);

    // split width into segments and draw lines
    for (int i = 0; i <= partitions * 2; i++) {
        SDL_Point start = {0, i * PLOT_LINE_SPACING};

        SDL_Point end = {i * (PLOT_LINE_SPACING), 0};
        
        SDL_RenderDrawLine(renderer, start.x, start.y, end.x, end.y);
    }

    SDL_SetRenderTarget(renderer, NULL);

    SDL_Rect src = {0, 0, effectiveWidth, effectiveHeight};
    SDL_Rect dst = {offx, offy, effectiveWidth, effectiveHeight};
    SDL_RenderCopy(renderer, renderTexture, &src, &dst);
    SDL_DestroyTexture(renderTexture);

    SDL_SetRenderDrawColor(renderer, 0xD0, 0x10, 0x10, 0xff);
    SDL_RenderDrawRect(renderer, &this->tmp);
}

// returns a bool indicating if the plot needs updating
void Plot::update(SDL_Point* mouse, SDL_Point* deltaMouse, std::vector<Plot*>& list) {
    this->mouse = *mouse;
    this->deltaMouse = *deltaMouse;
    this->renderTooltip = false;

    this->previousMouse = this->currentMouse;
    this->currentMouse = SDL_GetMouseState(NULL, NULL);

    this->mouseState = this->updateMouseState();
    if (this->mouseState != MouseState::OUT_OF_BOUNDS) {
        this->updatePosition(list);
    } 

    if (SDL_PointInRect(mouse, &this->bounds)) {
        this->renderTooltip = true;
    }
}

bool Plot::checkCollisions(Plot* plot, std::vector<Plot*>& list) {
    for (Plot* p : list) {
        if ((p != plot) && SDL_HasIntersection(&plot->bounds, &p->bounds)) {
            return true;
        }
    }

    return false;
}

void Plot::updatePosition(std::vector<Plot*>& plots) {
    int lastx = this->bounds.x;
    int lasty = this->bounds.y;

    switch (this->mouseState) {
        case MouseState::SELECTED_CENTER:
            this->move(this->deltaMouse.x, this->deltaMouse.y);
            break;
        case MouseState::SELECTED_TOP:
            this->move(0, this->deltaMouse.y);
            this->resize(0, -this->deltaMouse.y);
            this->bounds.h = this->bounds.h < PLOT_MIN_HEIGHT ? PLOT_MIN_HEIGHT : this->bounds.h;
            break;
        case MouseState::SELECTED_BOTTOM:
            this->resize(0, this->deltaMouse.y);
            this->bounds.h = this->bounds.h < PLOT_MIN_HEIGHT ? PLOT_MIN_HEIGHT : this->bounds.h;
            break;
        case MouseState::SELECTED_LEFT:
            this->move(this->deltaMouse.x, 0);
            this->resize(-this->deltaMouse.x, 0);
            this->bounds.w = this->bounds.w < PLOT_MIN_WIDTH ? PLOT_MIN_WIDTH : this->bounds.w;
            break;
        case MouseState::SELECTED_RIGHT:
            this->resize(this->deltaMouse.x, 0);
            this->bounds.w = this->bounds.w < PLOT_MIN_WIDTH ? PLOT_MIN_WIDTH : this->bounds.w;
            break;
    }

    if (this->checkCollisions(this, plots)) {
        this->bounds.x = lastx;
        this->bounds.y = lasty;
    }
}

void Plot::move(int deltaX, int deltaY) {
    this->bounds.x += deltaX;
    this->bounds.y += deltaY;
}

void Plot::resize(int deltaWidth, int deltaHeight) {
    this->bounds.w += deltaWidth;
    this->bounds.w = (this->bounds.w < PLOT_MIN_WIDTH) ? (PLOT_MIN_WIDTH) : (this->bounds.w);
    this->bounds.h += deltaHeight;
    this->bounds.h = (this->bounds.h < PLOT_MIN_HEIGHT) ? (PLOT_MIN_HEIGHT) : (this->bounds.h);
}

// VERY IMPORTANT!!!
// this function returns which region of the plot the mouse is in
MouseState Plot::updateMouseState() {
    /*
        NOTE: the collision rectangles are in clockwise order starting from the top side
        this HAS to be the same as the enum, otherwise the whole application will break
    */

    // since the updating rates between the mouse and sdl have different timings, the mouse delta
    // will always be less than what it really is
    // so, if the current mouse state is being held, skip collision detection, and just return the same
    if (this->mouseState >= MouseState::SELECTED_TOP && this->mouseState <= MouseState::SELECTED_CENTER) {
        if ((this->currentMouse & SDL_BUTTON_LMASK) && (this->previousMouse & SDL_BUTTON_LMASK)) {
            return this->mouseState;
        }
    }

    SDL_Rect rects[5] = {
        // collision rect for resizing the top side
        (SDL_Rect) {this->bounds.x, this->bounds.y - RESIZE_TOLERANCE, this->bounds.w, RESIZE_TOLERANCE * 2},
        // collision rect for resizing the right side
        (SDL_Rect) {this->bounds.x + this->bounds.w - RESIZE_TOLERANCE, this->bounds.y, RESIZE_TOLERANCE * 2, this->bounds.h},
        // collision rect for resizing bottom side
        (SDL_Rect) {this->bounds.x, this->bounds.y + this->bounds.h - RESIZE_TOLERANCE, this->bounds.w, RESIZE_TOLERANCE * 2},
        // collision rect for resizing left side
        (SDL_Rect) {this->bounds.x - RESIZE_TOLERANCE, this->bounds.y, RESIZE_TOLERANCE * 2, this->bounds.h},
        // collision rect for center of plot
        (SDL_Rect) {this->bounds.x + RESIZE_TOLERANCE, this->bounds.y + RESIZE_TOLERANCE, this->bounds.w - (RESIZE_TOLERANCE * 2), this->bounds.h - (RESIZE_TOLERANCE * 2)}
    };

    bool clicked = this->currentMouse & SDL_BUTTON_LMASK;

    for (int i = 0; i < 5; i++) {
        if (SDL_PointInRect(&this->mouse, &rects[i])) {
            // returns the appropriate mouse state
            // check enum for more info
            this->tmp = rects[i];
            return (MouseState) (i + (clicked ? 5 : 0));
        }
    }

    this->tmp = (SDL_Rect){0, 0, 1, 1,};
    return MouseState::OUT_OF_BOUNDS;
}

const char* Plot::stateAsString() {
    if (this->mouseState == MouseState::OUT_OF_BOUNDS) {
        return "MouseState::OUT_OF_BOUNDS";
    }

    return (const char*[]) {
        "MouseState::HOVERED_TOP",
        "MouseState::HOVERED_RIGHT",
        "MouseState::HOVERED_BOTTOM",
        "MouseState::HOVERED_LEFT",
        "MouseState::HOVERED_CENTER",
        "MouseState::SELECTED_TOP",
        "MouseState::SELECTED_RIGHT",
        "MouseState::SELECTED_BOTTOM",
        "MouseState::SELECTED_LEFT",
        "MouseState::SELECTED_CENTER",
    }[(int) this->mouseState];
}