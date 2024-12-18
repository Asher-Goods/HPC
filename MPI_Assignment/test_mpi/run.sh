#!/bin/bash
#SBATCH --job-name=mpitest
#SBATCH --nodes=2 #number of nodes
#SBATCH --ntasks-per-node=28
#SBATCH --cluster=mpi # mpi, gpu and smp are available in H2P
#SBATCH --time=10:00 # walltime in dd-hh:mm format
#SBATCH --qos=normal # enter long if walltime is greater than 3 days
#SBATCH --output=test.out # the name of the file that contains output
#SBATCH --account=ece1115_2024f
module purge #make sure the modules environment is sane
module load intel/2017.1.132 intel-mpi/2017.1.132
cp test_mpi.cpp $SLURM_SCRATCH
mpicxx test_mpi.cpp -std=c++11 -o main.o
mpirun -np $SLURM_NTASKS ./main.o # Run the runnable
