#include "TemperatureAnalysis.h"

int main() {
    // Create an instance of TemperatureAnalysis
    TemperatureAnalysis analysis;

    printf("STEP 1: Initialize File\n");

    // Initialize the file
    if (!analysis.initializeFile("bigw12a.log")) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }

    printf("STEP 1: COMPLETE\n\n");

    printf("STEP 2: Process temperature data\n");
    // Process the temperature data
    analysis.processTemperatureData();
    printf("STEP 2: COMPLETE\n\n");

    printf("STEP 3: Generate report\n");
    analysis.generateReport("outputData.log");
    printf("STEP 3: COMPLETE\n");

    return 0;
}
