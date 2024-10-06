#include "TemperatureAnalysis.h"

int main() {
    // Create an instance of TemperatureAnalysis
    TemperatureAnalysis analysis;

    // Initialize the file
    if (!analysis.initializeFile("biw12.log")) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }

    // Process the temperature data
    analysis.processTemperatureData();
    analysis.generateReport("outputData.log");

    return 0;
}
