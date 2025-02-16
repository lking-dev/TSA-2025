/*
 *  main.cpp - data loading and application entry point
 *  written for GATSA's SLC '25 Software Development event
*/

#include "main.hpp"

// entry point
int main(int argc, char** argv) {
    // create crop data manager and fill it with data
    CropRegistry* registry = new CropRegistry();
    registry->loadFromCSV("crop.csv");

    // input stream for farm data
    std::ifstream stream("farm.json", std::ifstream::binary);
    App* app;

    // if file exists, call with stream, otherwise nullptr
    if (stream.good()) {
        app = new App(registry, &stream);
    } else {
        app = new App(registry, nullptr);
    }
    
    // start app
    return app->run();
}
