#!/bin/bash

# SLURM job settings (default values will be overridden dynamically)
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=100G
#SBATCH --time=00:10:00

# Script arguments
MODE=$1         # Mode: "seq", "mpi", "openmp", or "all"
MATRIX_SIZE=$2  # Matrix size
DENSITY=$3      # Matrix density percentage
PROCESSES=$4    # Number of MPI processes or OpenMP threads
THREADS=$5      # Number of OpenMP threads if MPI+OpenMP, or 0 if only MPI
NODES=$6        # Number of nodes

# Determine the execution mode based on the provided mode argument
case "$MODE" in
    "seq")
        # Sequential execution (no MPI or OpenMP)
        EXECUTABLE="./project2 $MATRIX_SIZE $DENSITY"
        PROCESSES=1
        THREADS=1
        ;;
    "mpi")
        # Pure MPI
        EXECUTABLE="./project2 $MATRIX_SIZE $DENSITY $PROCESSES"
        THREADS=1
        ;;
    "openmp")
        # Pure OpenMP
        EXECUTABLE="./project2 $MATRIX_SIZE $DENSITY $PROCESSES"
        THREADS=$PROCESSES
        ;;
    "all")
        # MPI + OpenMP
        EXECUTABLE="./project2 $MATRIX_SIZE $DENSITY $PROCESSES $THREADS"
        ;;
    *)
        echo "Invalid mode. Please specify 'seq', 'mpi', 'openmp', or 'all'."
        exit 1
        ;;
esac

# Set the number of nodes dynamically
#SBATCH --nodes=${NODES}

# Compilation step based on the mode
if [ "$MODE" == "all" ]; then
    # MPI + OpenMP
    mpicxx -fopenmp -D_MPI -o project2 project2.cpp
elif [ "$MODE" == "mpi" ]; then
    # Pure MPI
    mpicxx -D_MPI -o project2 project2.cpp
elif [ "$MODE" == "openmp" ]; then
    # Pure OpenMP
    g++ -fopenmp -o project2 project2.cpp
else
    # Sequential
    g++ -o project2 project2.cpp
fi

# Loop for increasing matrix size until time limit exceeds
while true; do
  # Run the matrix multiplication based on the execution mode
  if [ "$MODE" == "all" ]; then
    # MPI + OpenMP
    srun --nodes=${NODES} --ntasks-per-node=${PROCESSES} --cpus-per-task=${THREADS} $EXECUTABLE > output.log
  elif [ "$MODE" == "mpi" ]; then
    # Pure MPI
    srun --nodes=${NODES} --ntasks-per-node=${PROCESSES} --cpus-per-task=1 $EXECUTABLE > output.log
  elif [ "$MODE" == "openmp" ]; then
    # Pure OpenMP
    srun --nodes=${NODES} --ntasks-per-node=1 --cpus-per-task=${THREADS} $EXECUTABLE > output.log
  else
    # Sequential
    srun $EXECUTABLE > output.log
  fi

  # Parse the output to get the execution time
  TIME=$(grep "Elapsed time:" output.log | awk '{print $4}')

  # Check if the time exceeds 600 seconds (10 minutes)
  if (( $(echo "$TIME > 600.0" | bc -l) )); then
    echo "Matrix size $MATRIX_SIZE exceeds 10 minutes. Stopping."
    break
  fi

  # Increase matrix size for the next iteration
  MATRIX_SIZE=$(($MATRIX_SIZE * 2))
  echo "Increasing matrix size to $MATRIX_SIZE"
done
