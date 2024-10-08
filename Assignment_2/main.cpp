#include <iostream>
#include <cstdio>
#include <sys/time.h>
#include "TemperatureAnalysis.h"

int main() {
    struct timeval start, end;

    // Create an instance of TemperatureAnalysis
    TemperatureAnalysis analysis;

    // Step 1 Initialize
    printf("STEP 1: Initialize File\n");
    gettimeofday(&start, NULL); // Start timer

    // Initialize the file
    if (!analysis.initializeFile("bigw12.log")) {
        std::cerr << "Error: Could not open file" << std::endl;
        return 1;
    }

    gettimeofday(&end, NULL); // Stop timer
    int micro_start = start.tv_sec * 1000000 + start.tv_usec;
    int micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 1 (Initialize): %d microseconds\n\n", micro_end - micro_start);
    printf("STEP 1: COMPLETE\n\n");

    // Step 2 Process
    printf("STEP 2: Process temperature data\n");
    gettimeofday(&start, NULL); // Start timer

    // Process temperature data
    analysis.processTemperatureData();

    gettimeofday(&end, NULL); // Stop timer
    micro_start = start.tv_sec * 1000000 + start.tv_usec;
    micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 2 (Process): %d microseconds\n\n", micro_end - micro_start);
    printf("STEP 2: COMPLETE\n\n");

    // Step 3 Generate Report
    printf("STEP 3: Generate report\n");
    gettimeofday(&start, NULL); // Start timer

    // Generate the report
    analysis.generateReport("outputData.log");

    gettimeofday(&end, NULL); // Stop timer
    micro_start = start.tv_sec * 1000000 + start.tv_usec;
    micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 3 (Generate Report): %d microseconds\n\n", micro_end - micro_start);
    printf("STEP 3: COMPLETE\n");

    return 0;
}
