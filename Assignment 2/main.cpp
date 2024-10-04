#include "TemperatureAnalysis.h"

int main() {
    // Create an instance of TemperatureAnalysis
    TemperatureAnalysis analysis;

    // Initialize the file
    if (!analysis.initializeFile("temperature_data.txt")) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }

    // Process the temperature data
    analysis.processTemperatureData();

    return 0;
}
