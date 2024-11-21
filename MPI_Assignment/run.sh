#!/bin/bash
#SBATCH --job-name=mpitest           # Job name
#SBATCH --nodes=5                    # Allocate 5 nodes
#SBATCH --ntasks=5                   # Use 5 MPI tasks
#SBATCH --ntasks-per-node=1          # One MPI task per node
#SBATCH --cpus-per-task=28           # Request all 28 cores on each node for your task
#SBATCH --cluster=mpi                # Specify MPI cluster
#SBATCH --time=10:00                 # Walltime
#SBATCH --qos=normal                 # 'long' for walltime > 3 days
#SBATCH --output=test.out            # Standard output file
#SBATCH --error=test.err             # Standard error file
#SBATCH --account=ece1115_2024f

# Purge any existing modules to ensure a clean environment
module purge

# Load the necessary modules
module load intel/2017.1.132 intel-mpi/2017.1.132

# Copy your source files to the SLURM scratch directory
cp main.cpp $SLURM_SCRATCH
cp TemperatureAnalysisMPI.cpp $SLURM_SCRATCH
cp TemperatureAnalysisMPI.h $SLURM_SCRATCH
cp bigw12a.log $SLURM_SCRATCH

# Compile the source files into object files
# Compile and link all the source files in one step
mpicxx main.cpp TemperatureAnalysisMPI.cpp -o main -std=c++11

# Run the executable with the number of MPI tasks specified by SLURM
mpirun -np $SLURM_NTASKS ./main
