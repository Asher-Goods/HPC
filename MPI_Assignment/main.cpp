#include "TemperatureAnalysisMPI.h"
#include <mpi.h>

int main(int argc, char *argv[]) {
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
    return 0;
}