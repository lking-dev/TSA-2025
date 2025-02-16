/*
 *  main.hpp - shared header file for all code
 *  written for GATSA's SLC '25 Software Development event
*/

// include sdl2 for window management and rendering
// https://www.libsdl.org/
#include <SDL2/SDL.h>

// include imgui (intermediate mode gui) library for gui usage
// https://github.com/ocornut/imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

// include fastcsv, for csv reading
// https://github.com/ben-strasser/fast-cpp-csv-parser/tree/master
#define CSV_IO_NO_THREAD
#include "FastCSV/csv.h"

// include jsoncpp for json serialization and deserialization
// https://github.com/open-source-parsers/jsoncpp
#include "json/json.h"

// include the c++ std library
#include <bits/stdc++.h>

// window width and height definitions
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 680

// random definitions
#define PLOT_LINE_SPACING 10
#define PLOT_PADDING (PLOT_LINE_SPACING / 2)
#define PLOT_MIN_WIDTH 64
#define PLOT_MIN_HEIGHT 64
#define SIDE_PANEL_WIDTH (0.2)

class App;
struct Plot;
class CropRegistry;

class App {
private:
    // SDL2 related
    SDL_Window* window;
    SDL_Renderer* renderer;

    // app variables
    bool closed;
    std::vector<Plot*> plots;
    Plot* selectedPlot;
    CropRegistry* registry;
    SDL_Point mouse;
    SDL_Point deltaMouse;
    std::string farmName;
    int plotCount;

    // assets
    SDL_Cursor* handCursor;
    SDL_Cursor* arrowCursor;
    SDL_Cursor* resizeVCursor;
    SDL_Cursor* resizeHCursor;

public:
    // constructor, takes care of initializing SDL2 and IMGUI
    App(CropRegistry* registry, std::ifstream* src);
    // destructor, destroys window and closes SDL2 and IMGUI contexts
    ~App();

public:
    // application mainloop
    int run();

private:
    // general updates
    void update();
    // updating cursor icon
    void updateCursor();
    // rendering the scene
    void renderEngine();
    // adds a new plot
    void addNewPlot(Plot* plot);
    // saves farm data to .json file
    void saveFarm(std::string filename);

private:
    // load cursor icons
    void loadAssets();
};

// manager for holding all the information for a specific crop
class CropRegistry {
public:
    // subclass for holding data in singular object for data table
    struct CropEntry {
        // name of crop
        std::string name;
        // average yield in lbs/plant
        // important! it is not in bsh/ac or kg/ha
        double avgYield;
        // color to be displayed for each plot
        SDL_Color color;

        CropEntry(std::string name, double avgYield, int red, int green, int blue);
    };

    // actual data being stored, using crop name as key in hash table
    std::unordered_map<std::string, CropEntry*> registry;

public:
    CropRegistry();

private:
    ~CropRegistry();

public:
    // add a new entry
    void addEntry(std::string name, double yield, int red, int green, int blue);
    // load crop data from a csv table
    void loadFromCSV(std::string filename);
    // get a crop's data from its name
    CropEntry* access(std::string name);
    // get list of crops stored
    std::vector<std::string> getKeyList();
    // get list of crops stored, but in char*'s instead of std::string
    char** getKeyListAsCSTRS();
    // free allocated memory for char* array from previous function
    void freeCSTRS(char** list, int size);
};

struct Plot {
    // sdl properties
    SDL_Rect bounds;
    SDL_Color color;
    SDL_Rect tmp;

    // mouse data
    int currentMouse;
    int previousMouse;

    // copy of main app's information
    SDL_Point mouse;

    // plot data
    bool windowOpen;
    SDL_Point windowPos;
    char plotName[128];
    int id;

    // crop data
    std::string cropName;
    int cropIndex;
    float expectedYield;
    float yieldDeviance;

    // constructor with crop, plot, and scene data
    // crop index is more needed for the imgui combo box than anything else
    Plot(int x, int y, int width, int height, std::string name, int cropIndex, double cropDeviation, CropRegistry::CropEntry* crop);

    // update a plot
    bool update();
    // up[date a plot when its not selected, basically a smaller update versin
    void updateNonSelected(bool forceNoUpdate);
    // test if plot is hpvered
    bool isHovered();
    // test if plot is selected
    bool isSelected();
    // test if point (usually the mouse point) is in the plot's bounding box
    bool inBounds(const SDL_Point* p);
    // move the plot when dragged
    void updatePosition(SDL_Point* deltaMouse, std::vector<Plot*>& plots);
    // move plot from gui updates
    void updateFromInputs(int xin, int yin, int win, int hin, std::vector<Plot*>& plots);
    // update crop data when changed
    void updateProperties(CropRegistry::CropEntry* entry, int index);
    // register when the plot has been right clicked, returns true if it has been and false otherwise
    bool registerClick(const SDL_Point* p);
    // render the plot
    void render(SDL_Renderer* renderer);
    // move the plot in a direction
    void move(int deltaX, int deltaY);
    // check for any bounding box collisions with other plots
    bool checkCollisions(std::vector<Plot*>& list);
};