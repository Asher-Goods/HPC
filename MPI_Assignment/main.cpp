#include "TemperatureAnalysisMPI.h"
#include <mpi.h>
#include <sys/time.h>
#include <iostream>

int main(int argc, char *argv[]) {
    struct timeval start, end;
    gettimeofday(&start, NULL); // Start timer
    MPI_Init(&argc, &argv);

    TemperatureAnalysisMPI analysis;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    string inputFile = "bigw12a.log";
    string outputFile = "outputData.log";

    analysis.setHeatingMonths({12, 1, 2, 3});
    analysis.setCoolingMonths({7, 8, 9});

    if (rank == FILEREADER) {
        analysis.fileReader(inputFile);
    } else if (rank == PARSER) {
        analysis.parser();
    } else if (rank == ANOMALYDETECTOR) {
        analysis.anomalyDetector(); 
    } else if (rank == EVALUATETEMPERATURES) {
        analysis.evaluateMonthlyTemperatures();
    } else if (rank == FILEWRITER) {
        analysis.fileWriter(outputFile);
    }

    MPI_Finalize();

    gettimeofday(&end, NULL); // End timer

    // Convert microseconds to seconds
    double elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Total time for rank %d: %.6f seconds\n", rank, elapsedTime);

    return 0;
}
