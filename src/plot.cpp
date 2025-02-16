/*
 *  plot.cpp - farm plot management and rendering code
 *  written for GATSA's SLC '25 Software Development event
*/

#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "main.hpp"

// plot constructor, takes in crop, size, and design information
Plot::Plot(int x, int y, int width, int height, std::string name, int cropIndex, double cropDeviation, CropRegistry::CropEntry* crop) {
    this->bounds = (SDL_Rect){x, y, width, height};
    this->windowOpen = false;

    // copy name over into char buffer for imgui input
    memset(this->plotName, 0, sizeof(this->plotName));
    strcpy(this->plotName, name.c_str());

    // get crop information from registry field
    this->cropName = crop->name;
    this->cropIndex = cropIndex;
    this->expectedYield = crop->avgYield;
    this->yieldDeviance = cropDeviation;
    this->color = crop->color;
}

void Plot::render(SDL_Renderer* renderer) {
    // draw the outline of the plot different colors based on selection/hovering
    if (this->isSelected()) {
        // almost white
        SDL_SetRenderDrawColor(renderer, 0xD0, 0xD0, 0xD0, 0xFF);
    } else if (this->isHovered()) {
        // light gray
        SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF);
    } else {
        // dark gray
        SDL_SetRenderDrawColor(renderer, 0x40, 0x40, 0x40, 0xFF);
    }

    // draw the bounding box
    SDL_RenderDrawRect(renderer, &this->bounds);

    // set rendering color to color of plot
    SDL_SetRenderDrawColor(renderer, this->color.r, this->color.g, this->color.b, this->color.a);
    // calculate rendering offsets
    int offx = this->bounds.x + PLOT_PADDING;
    int offy = this->bounds.y + PLOT_PADDING;
    int effectiveWidth = this->bounds.w - (PLOT_PADDING * 2);
    int effectiveHeight = this->bounds.h - (PLOT_PADDING * 2);
    int largestBound = effectiveWidth > effectiveHeight ? effectiveWidth : effectiveHeight;
    int partitions = largestBound / PLOT_LINE_SPACING;
    
    // create render texture for rendering
    SDL_Texture* renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        effectiveWidth * 2, effectiveHeight * 2);
    // target the created texture
    SDL_SetRenderTarget(renderer, renderTexture);

    // split width into segments and draw lines diagonally
    for (int i = 0; i <= partitions * 2; i++) {
        SDL_Point start = {0, i * PLOT_LINE_SPACING};
        SDL_Point end = {i * (PLOT_LINE_SPACING), 0};
    
        SDL_RenderDrawLine(renderer, start.x, start.y, end.x, end.y);
    }

    // set render target back to main window
    SDL_SetRenderTarget(renderer, NULL);

    // render our plot onto the screen
    SDL_Rect src = {0, 0, effectiveWidth, effectiveHeight};
    SDL_Rect dst = {offx, offy, effectiveWidth, effectiveHeight};
    SDL_RenderCopy(renderer, renderTexture, &src, &dst);
    SDL_DestroyTexture(renderTexture);

    SDL_SetRenderDrawColor(renderer, 0xD0, 0x10, 0x10, 0xff);
    SDL_RenderDrawRect(renderer, &this->tmp);
}

// update the plot when its not selected, can optionally ignore all mouse input
void Plot::updateNonSelected(bool forceNoUpdate) {
    this->previousMouse = this->currentMouse;
    this->currentMouse = SDL_GetMouseState(&this->mouse.x, &this->mouse.y);

    // force mouse input to be ignored
    if (forceNoUpdate) {
        this->currentMouse = 0;
        this->mouse = (SDL_Point){0, 0};
    }
}

// returns true/false based on if a right click was in the plots bounds
bool Plot::registerClick(const SDL_Point* p) {
    if (SDL_PointInRect(p, &this->bounds)) {
        // toggle window being open
        this->windowOpen = !this->windowOpen;
        this->windowPos = *p;
        return true;
    }

    return false;
}

bool Plot::update() {
    // update the basic fields
    this->updateNonSelected(false);

    // return if the plot is in focus
    return this->isSelected() || this->isHovered();
}

// returns true if the plot is being held with left click
bool Plot::isSelected() {
    return (SDL_PointInRect(&this->mouse, &this->bounds) && (this->currentMouse & SDL_BUTTON_LMASK));
}

// retruns true if plot contains mouse
bool Plot::isHovered() {
    return (SDL_PointInRect(&this->mouse, &this->bounds));
}

// returns true if any point is in bounding box
bool Plot::inBounds(const SDL_Point* p) {
    return SDL_PointInRect(p, &this->bounds);
}

// checks for bounding box collisions with other plots
bool Plot::checkCollisions(std::vector<Plot*>& list) {
    for (auto& p : list) {
        // ignore itself in the check
        if ((p != this) && SDL_HasIntersection(&this->bounds, &p->bounds)) {
            return true;
        }
    }

    return false;
}

// update the plots position
void Plot::updatePosition(SDL_Point* deltaMouse, std::vector<Plot*>& plots) {
    // save last locations
    int lastx = this->bounds.x;
    int lasty = this->bounds.y;

    // try moving
    this->move(deltaMouse->x, deltaMouse->y);

    // if touching other plots then reset position
    if (this->checkCollisions(plots)) {
        this->bounds.x = lastx;
        this->bounds.y = lasty;
    }

    // if out of screen
    if (this->bounds.x < 0 || this->bounds.x > WINDOW_WIDTH - this->bounds.w) {
        this->bounds.x = lastx;
    }

    if (this->bounds.y < 0 || this->bounds.y > WINDOW_HEIGHT - this->bounds.h) {
        this->bounds.y = lasty;
    }
}

// updates the plots position from the gui inputs, instead of mouse dragging
void Plot::updateFromInputs(int xin, int yin, int win, int hin, std::vector<Plot*>& plots) {
    // update left-right position
    if (xin != this->bounds.x) {
        int oldx = this->bounds.x;
        this->bounds.x = xin;
        if (this->checkCollisions(plots)) {
            this->bounds.x = oldx;
        }
    }

    // update up-down position
    if (yin != this->bounds.y) {
        int oldy = this->bounds.y;
        this->bounds.y = yin;
        if (this->checkCollisions(plots)) {
            this->bounds.y = oldy;
        }
    }

    // update the width
    if (win != this->bounds.w) {
        int oldw = this->bounds.w;
        this->bounds.w = win;
        if (this->checkCollisions(plots)) {
            this->bounds.w = oldw;
        }
    }

    // update the height
    if (hin != this->bounds.h) {
        int oldh = this->bounds.h;
        this->bounds.h = hin;
        if (this->checkCollisions(plots)) {
            this->bounds.h = oldh;
        }
    }
}

// resets all the plot's properties based on a new crop selection
void Plot::updateProperties(CropRegistry::CropEntry* entry, int index) {
    this->cropName = entry->name;
    this->cropIndex = index;
    this->expectedYield = entry->avgYield;
    this->color = entry->color;
}

// moves the plot
void Plot::move(int deltaX, int deltaY) {
    this->bounds.x += deltaX;
    this->bounds.y += deltaY;
}