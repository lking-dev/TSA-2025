/*
 *  registry.hpp - crop data for common use
 *  written for GATSA's SLC '25 Software Development event
*/

#include "main.hpp"

// constructor for creating hashmap
CropRegistry::CropRegistry() {
    this->registry = std::unordered_map<std::string, CropEntry*>();
}

// constructor for CropEntry subclass
CropRegistry::CropEntry::CropEntry(std::string name, double avgYield, int red, int green, int blue) {
    this->name = name;
    this->avgYield = avgYield;
    this->color = (SDL_Color){(char) red, (char) green, (char) blue, 0xFF};
}

// deletes the allocated crop entries
CropRegistry::~CropRegistry() {
    for (const auto& pair : this->registry) {
        delete pair.second;
    }
}

// adds new entry to the hashmap if it isnt already added
void CropRegistry::addEntry(std::string name, double yield, int red, int green, int blue) {
    if (this->registry.find(name) == this->registry.end()) {
        this->registry[name] = new CropRegistry::CropEntry(name, yield, red, green, blue);
    }
}

void CropRegistry::loadFromCSV(std::string filename) {
    // read from default csv file, by default should be "crop.csv"
    // format: crop-name, crop yield, r, g, b
    io::CSVReader<5> csvReader(filename);
    csvReader.read_header(io::ignore_missing_column, "name", "yield", "red", "green", "blue");

    // fields to read into, passed by reference
    std::string name;
    double yield;
    int red, green, blue;

    // read in all the data and add entries to table
    while (csvReader.read_row(name, yield, red, green, blue)) {
        this->addEntry(name, yield, red, green, blue);
    }

    // adds a type for null selections at the end, so it has an index of 0
    this->addEntry("NO SELECTION", 0.0, 120, 120, 120);
}

// returns the list of crops stored in the table
std::vector<std::string> CropRegistry::getKeyList() {
    std::vector<std::string> list;
    
    for (const auto& pair : this->registry) {
        list.push_back(pair.first);
    }
    
    return list;
}

// returns the list of crops stored as array of char*'s
char** CropRegistry::getKeyListAsCSTRS() {
    // get the initial std::string list
    std::vector<std::string> list = this->getKeyList();
    // allocate space for the pointers needed
    char** strings = (char**) malloc(sizeof(char*) * list.size());

    // strings stored MUST BE NEW COPYS
    // you cannot use the std::string.c_str()
    // that memory is volitile and cannot be used or returned
    int index = 0;
    for (auto& s : list) {
        int strSize = s.size();
        strings[index] = (char*) malloc(sizeof(char) * strSize);
        strcpy(strings[index], s.c_str());
        index++;
    }

    return strings;
}

// free the memory allocated for the c strings
void CropRegistry::freeCSTRS(char** list, int size) {
    // TOTO UNTIL I FUGURE OUT WHAT I DID WRONG
    return;

    for (int i = 0; i < size; i++) {
        free(list[i]);
    }

    free(list);
    list = nullptr;
}

// get crop data based on the crop's name
CropRegistry::CropEntry* CropRegistry::access(std::string name) {
    // if the crop isnt found, return nullptr
    if (this->registry.find(name) == this->registry.end()) {
        return nullptr;
    // return the crop data if found
    } else {
        return this->registry[name];
    }
}