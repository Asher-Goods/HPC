#include <iostream>
#include <cstdio>
#include <sys/time.h>
#include "TemperatureAnalysisParallel.h"

int main() {
    struct timeval start, end;

    std::string filename = "bigw12a.log";
    std::string outputFile = "outputData.log";

    printf("Initialize File and Setup Pipeline\n");
    gettimeofday(&start, NULL); // Start timer

    TemperatureAnalysisParallel analysis(filename);
    analysis.setHeatingMonths({12, 1, 2, 3});
    analysis.setCoolingMonths({7, 8, 9});

    // Initialize pipeline and start all stages as threads
    analysis.startPipeline(outputFile);

    gettimeofday(&end, NULL); // Stop timer
    int micro_start = start.tv_sec * 1000000 + start.tv_usec;
    int micro_end = end.tv_sec * 1000000 + end.tv_usec;
    printf("Total time for initializing and processing with pipeline: %d microseconds\n\n", micro_end - micro_start);

    return 0;
}
