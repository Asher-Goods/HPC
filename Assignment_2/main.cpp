#include "TemperatureAnalysis.h"

int main() {
    // Create an instance of TemperatureAnalysis
    TemperatureAnalysis analysis;

    // Step 1 Initialize
    printf("STEP 1: Initialize File\n");
    // Initialize the file
    if (!analysis.initializeFile("bigw12.log")) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }
    printf("STEP 1: COMPLETE\n\n");

    // Step 2 Process
    printf("STEP 2: Process temperature data\n");
    analysis.processTemperatureData();
    printf("STEP 2: COMPLETE\n\n");

    // Step 3 Generate Report
    printf("STEP 3: Generate report\n");
    analysis.generateReport("outputData.log");
    printf("STEP 3: COMPLETE\n");

    return 0;
}
