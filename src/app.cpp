/*
 *  app.cpp - application logic, rendering and mainloop
 *  written for GATSA's SLC '25 Software Development event
*/

#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "main.hpp"

// constructor for main application
App::App(CropRegistry* registry, std::ifstream* src) {
    // setup sdl2 context
    // returns 0 on success, so should fail
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        printf("FATAL ERROR: UNABLE TO INITALIZE SDL2\n");
        printf("ERROR MESSAGE: %s\n", SDL_GetError());
        return;
    }

    // creates the sdl window context
    this->window = SDL_CreateWindow("Farm Planner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (this->window == NULL) {
        printf("FATAL ERROR: UNABLE TO CREATE WINDOW CONTEXT\n");
        printf("ERROR MESSAGE: %s\n", SDL_GetError());
        return;
    }

    // creates sdl renderer context
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
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
    ImGui::GetIO().IniFilename = nullptr;

    // setup class variables
    this->closed = false;
    this->plots = std::vector<Plot*>();
    this->selectedPlot = nullptr;
    this->registry = registry;

    // loading farm setup if there is no file found
    if (src == nullptr) {
        this->farmName = std::string("UNAMED FARM");
        this->plotCount = 0;
    } else {
        // loading from a file, farm.json
        Json::Reader reader;
        Json::Value dst;
        // read everything into dst as main object
        reader.parse(*src, dst);

        this->farmName = dst["name"].asString();
        // size holds number of plots, size IS NOT the same as this->farmsize
        int size = dst["size"].asInt();
        Json::Value& plots = dst["plots"];

        // load in every plot from the array
        for (int i = 0; i < size; i++) {
            // bounding box infomation
            int x = plots[i]["x"].asInt();
            int y = plots[i]["y"].asInt();
            int w = plots[i]["width"].asInt();
            int h = plots[i]["height"].asInt();

            // plot/crop information
            std::string name = plots[i]["name"].asString();
            std::string crop = plots[i]["crop"].asString();
            int cropIndex = plots[i]["cropIndex"].asInt();
            double deviation = plots[i]["deviation"].asDouble();

            // create new plot with data
            CropRegistry::CropEntry* entry = this->registry->access(crop);
            Plot* plot = new Plot(x, y, w, h, name, cropIndex, deviation, entry);
            this->addNewPlot(plot);
        }
    }

    // load assets
    this->loadAssets();
}

void App::saveFarm(std::string filename) {
    // ignore saving if empty context
    if (this->plotCount < 1) {
        return;
    }

    // main object to be serialized
    Json::Value farmData;
    farmData["name"] = this->farmName;
    farmData["size"] = this->plotCount;

    // json array holding objects which are the actual plots
    Json::Value plots(Json::arrayValue);

    for (auto& plot : this->plots) {
        Json::Value plotData;

        // serialize all the data
        plotData["name"] = plot->plotName;
        plotData["x"] = plot->bounds.x;
        plotData["y"] = plot->bounds.y;
        plotData["width"] = plot->bounds.w;
        plotData["height"] = plot->bounds.h;
        plotData["crop"] = plot->cropName;
        plotData["cropIndex"] = plot->cropIndex;
        plotData["deviation"] = plot->yieldDeviance;

        // add to list
        plots.append(plotData);
    }

    // asign list as a field
    farmData["plots"] = plots;

    // serialize
    std::ofstream file(filename);
    file << farmData;
    file.close();
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
    // main loop, the application lives out of this function
    while (!this->closed) {
        // update not only has the application logic, but also all the GUI rendering code
        // why? its just how imgui works. all the gui calls have to be done in update
        this->update();
        this->updateCursor();

        // black draw color for clearing screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        // render the plots
        this->renderEngine();
        
        //ImGuiIO& io = ImGui::GetIO();
        //SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    return 0;
}

void App::update() {
    // mouse information
    SDL_Event event;
    SDL_Point lastmouse = this->mouse;
    SDL_GetMouseState(&this->mouse.x, &this->mouse.y);

    // the SDL2 event for mouse movement was kind of slow
    // so the deltamouse calculations are done here instead
    this->deltaMouse.x = this->mouse.x - lastmouse.x;
    this->deltaMouse.y = this->mouse.y - lastmouse.y;

    // processing the sdl events
    while (SDL_PollEvent(&event)) {
        // important! pass to imgui first
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            this->closed = true;
        }

        // testing for right click
        else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_RIGHT) {
                for (auto& plot : this->plots) {
                    // if one of the plots was clicked
                    if (plot->registerClick(&this->mouse)) {
                        // we close all other windows save for the one that just opened
                        for (auto& ptmp : this->plots) {
                            if (ptmp != plot) {
                                ptmp->windowOpen = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // important boilerplate for imgui
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    char outlineNameBuffer[128] = {};
    strcpy(outlineNameBuffer, this->farmName.c_str());

    // the side window with all the baseline information
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH * SIDE_PANEL_WIDTH, WINDOW_HEIGHT));
    ImGui::Begin("Farm", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SeparatorText("Farm Properties");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6);
        ImGui::InputText("Farm Name", outlineNameBuffer, 128);
        this->farmName = std::string(outlineNameBuffer);

        if (ImGui::Button("Save To Disk")) {
            this->saveFarm("farm.json");
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4);
        ImGui::Text("Crop Data Source: %s", "crop.csv");

        ImGui::SeparatorText("Farm Contents");
        ImGui::Text("Total Plots: %d", this->plotCount);

        // for treenodes to work, they need unique id's
        // so this counter and the following lambda make new id's everytime its called
        int nodeCounter = 0;
        auto fakeid = [&]() { return std::string("node" + std::to_string(nodeCounter++)).c_str(); };

        ImGui::BeginChild("content_tree");
        {
            // using table for a list of treenodes
            // so no top level treenode
            ImGui::BeginTable("table", 1);
            {
                for (auto& p : this->plots) {
                    // next column must be called as well
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();


                    // the treenode status is saved in a variable instead of being consumed directly
                    auto open = ImGui::TreeNode(fakeid(), "%s", p->plotName);

                    // this way we can test if its been clicked, and also test if its open later
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                        p->windowOpen = !p->windowOpen;
                        for (auto& ptmp : this->plots) {
                            if (ptmp != p) {
                                ptmp->windowOpen = false;
                            }
                        }
                    }

                    ImGui::Unindent();

                    // display extra information about plot
                    if (open) {
                        ImGui::TreeNodeEx(fakeid(), ImGuiTreeNodeFlags_Leaf, "Crop: %s", p->cropName.c_str());
                        ImGui::TreePop();
                        ImGui::TreeNodeEx(fakeid(), ImGuiTreeNodeFlags_Leaf, "Position: (%d, %d)", p->bounds.x, p->bounds.y);
                        ImGui::TreePop();
                        ImGui::TreeNodeEx(fakeid(), ImGuiTreeNodeFlags_Leaf, "Size: (%d, %d)", p->bounds.w, p->bounds.h);
                        ImGui::TreePop();
                        ImGui::TreePop();
                    }

                    ImGui::Indent();
                }
            }

            ImGui::EndTable();

            if (ImGui::Button("New Plot")) {
                Plot* newPlot = new Plot(500, 500, 50, 50, "UNAMED PLOT", 0, 0.0, this->registry->access("NO SELECTION"));
                this->addNewPlot(newPlot);
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();

    // state variable to determine if a click was made inside the gui or the app 
    bool passInputs = true;

    // imgui combo boxes need all the options in char* format not std::string
    // so the first array is an array of char*
    // the second is a vector of std::string
    char** cropOptions = this->registry->getKeyListAsCSTRS();
    std::vector<std::string> keyList = this->registry->getKeyList();
    int optionCount = keyList.size();

    for (auto& plot : this->plots) {
        // if the plot has its config window open
        if (plot->windowOpen) {
            // fields that will be pointed to by inputs
            // important that plot fields are not modified directly
            int inputXCoord = plot->bounds.x;
            int inputYCoord = plot->bounds.y;
            int inputWidth = plot->bounds.w;
            int inputHeight = plot->bounds.h;
            int selection = plot->cropIndex;
        
            ImGui::SetNextWindowSize(ImVec2(320, 270));
            ImGui::Begin("Plot Configuration", &plot->windowOpen, ImGuiWindowFlags_NoResize);
            {
                ImGui::SeparatorText("Properties");
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6);
                ImGui::InputText("Plot Name", plot->plotName, sizeof(plot->plotName));
                ImGui::InputInt("Position X", &inputXCoord, 10);
                ImGui::InputInt("Position Y", &inputYCoord, 10);
                ImGui::InputInt("Width", &inputWidth, 10);
                ImGui::InputInt("Height", &inputHeight, 10);

                ImGui::SeparatorText("Crop Information");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6);

                // dont display any extra crop info if no crop selected
                if (selection == 0) {
                    ImGui::Combo("Crop", &selection, cropOptions, optionCount);
                } else {
                    ImGui::Combo("Crop", &selection, cropOptions, optionCount);
                    ImGui::InputFloat("Expected Yield", &plot->expectedYield, 0.0, 0.0, "%.1f lbs/plant");
                    ImGui::DragFloat("Yield Deviance", &plot->yieldDeviance, 0.1, 0.0, 100.0, "%.1f%%");
                }

                ImGui::SeparatorText("Actions");

                if (ImGui::IsWindowHovered()) {
                    passInputs = false;
                }
            }

            ImGui::End();

            // if the crop name is different than update the crop information
            if (plot->cropName.compare(cropOptions[selection])) {
                CropRegistry::CropEntry* entry = this->registry->access(keyList.at(selection));
                plot->updateProperties(entry, selection);
            }

            // more updating
            plot->updateFromInputs(inputXCoord, inputYCoord, inputWidth, inputHeight, this->plots);
        }
    }

    // free all the memory allocated for the char* array
    this->registry->freeCSTRS(cropOptions, optionCount);

    // render all widgets
    ImGui::Render();

    // if the plots were actually selected and not the gui
    if (passInputs) {
        // updating when no plot selection
        if (this->selectedPlot == nullptr) {
            for (auto plot : this->plots) {
                // call update for each plot to find out if its been selected
                bool isSelected = plot->update();
                if (isSelected) {
                    // if selected than save selection
                    this->selectedPlot = plot;
                    break;
                }
            }
        }

        // updating when plot selected : ignore all others
        else {
            // special update for when certain things dont need to be updated
            for (auto plot : this->plots) {
                plot->updateNonSelected(false);
            }

            // full update the selected plot
            this->selectedPlot->update();

            // if selected plot no longer selected than set it back to nullptr
            if (!(this->selectedPlot->isSelected() || this->selectedPlot->isHovered())) {
                this->selectedPlot = nullptr;
            }
        }

        // if we still have a selected plot, move it based on delatMouse
        if (this->selectedPlot && this->selectedPlot->isSelected()) {
            this->selectedPlot->updatePosition(&this->deltaMouse, this->plots);
        }
    } else {
        this->selectedPlot = nullptr;
        for (auto plot : this->plots) {
            plot->updateNonSelected(true);
        }
    }

    // update the cursor icon based on whats happening in updates
    this->updateCursor();
}

// adds a new plot to list and manages the plot count
void App::addNewPlot(Plot* plot) {
    this->plots.push_back(plot);
    this->plotCount = this->plots.size();
}

// set the cursor to appropriate pointer
void App::updateCursor() {
    if (this->selectedPlot == nullptr) {
        SDL_SetCursor(this->arrowCursor);
        return;
    }

    if (this->selectedPlot->isSelected()) {
        SDL_SetCursor(this->arrowCursor);
    } else {
        SDL_SetCursor(this->handCursor);
    }
}

// render all the plots in the scene
void App::renderEngine() {
    for (auto plot : this->plots) {
        plot->render(this->renderer);
    }
}

// loads all the sdl cursors
void App::loadAssets() {
    this->handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    this->arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
}