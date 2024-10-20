#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <sys/time.h>
#include "TemperatureAnalysis.h"




int main() {
    struct timeval start, end;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Step 1 Initialize ////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create an instance of TemperatureAnalysis
    string filename = "bigw12.log";
    printf("STEP 1: Initialize File\n");
    gettimeofday(&start, NULL); // Start timer

    TemperatureAnalysis analysis(filename);
    
    gettimeofday(&end, NULL); // Stop timer
    int micro_start = start.tv_sec * 1000000 + start.tv_usec;
    int micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 1 (Initialize): %d microseconds\n\n", micro_end - micro_start);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Step 2 Process ///////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    printf("STEP 2: Process temperature data\n");
    gettimeofday(&start, NULL); // Start timer
    
    analysis.processTemperatureData();

    gettimeofday(&end, NULL); // Stop timer
    micro_start = start.tv_sec * 1000000 + start.tv_usec;
    micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 2 (Process): %d microseconds\n\n", micro_end - micro_start);
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Step 3 Generate Report ///////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    printf("STEP 3: Generate report\n");
    gettimeofday(&start, NULL); // Start timer

    string outputFile = "outputData.log";
    // analysis.calculateMonthlyMeans();
    analysis.generateReport(outputFile);

    gettimeofday(&end, NULL); // Stop timer
    micro_start = start.tv_sec * 1000000 + start.tv_usec;
    micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for STEP 3 (Generate Report): %d microseconds\n\n", micro_end - micro_start);

    return 0;
}
