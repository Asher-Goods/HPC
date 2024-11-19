#include "TemperatureAnalysisParallel.h"
#include <mpi.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure there are exactly 4 processes
    if (size != 4) {
        if (rank == 0) {
            cerr << "This program requires exactly 4 MPI processes.\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Input and output file paths
    const string inputFile = "bigw12a.txt";
    const string outputFile = "bigw12a.log";

    // Instantiate the pipeline class and start processing
    TemperatureAnalysisParallel pipeline(inputFile);
    pipeline.startPipeline(outputFile);

    MPI_Finalize();
    return 0;
}
