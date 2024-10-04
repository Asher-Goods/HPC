#include "TemperatureAnalysis.h"

using namespace std;


TemperatureAnalysis::TemperatureAnalysis() {}

TemperatureAnalysis::~TemperatureAnalysis() {
    if (inputFile.is_open()) {
        inputFile.close();
    }
}

// Used to initialize and open the file
bool TemperatureAnalysis::initializeFile(const string& filename) {
    inputFile.open(filename);
    return inputFile.is_open();
}

/**
 * Main method of the class which invokes other private methods to 
 * parse and calculate the data
 */
void TemperatureAnalysis::processTemperatureData(void) {

}

// Parses a line of temperature data
TemperatureData TemperatureAnalysis::parseLine(const std::string& line) {
    stringstream stream(line);
    string date;
    string time;
    double temp;
    stream >> date >> time >> temp;
    return TemperatureData(date, time, temp);
}


