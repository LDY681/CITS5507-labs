#!/bin/bash
# --mem=220G --time=08:00:00
#SBATCH --mem=100G
#SBATCH --time=01:00:00
#SBATCH --partition=work
#SBATCH --account=courses0101

# Arguments
MODE=$1         # Mode: seq, openmp, mpi, or hybrid
SIZE=$2         # Starting Matrix size (SIZE x SIZE)
PERCENT=$3      # Matrix percentage
ARG3=$4         # Threads (OpenMP) or Processes (MPI)
ARG4=$5         # Threads (OpenMP for hybrid)

echo "[SBATCH] Started with MODE=$MODE, SIZE=$SIZE, PERCENT=$PERCENT, ARG3=$ARG3, ARG4=$ARG4"

# Set OpenMP environment variables for optimal thread binding
export OMP_PROC_BIND=spread  # Ensure threads are spread across cores
export OMP_PLACES=cores      # Bind each thread to a specific core

# For small increment size
# INCREMENT=1000

# Compile the program based on different mode
if [ "$MODE" == "seq" ]; then
    # Sequential mode
    g++ -o project2 project2.c

elif [ "$MODE" == "openmp" ]; then
    # Pure OpenMP mode
    g++ -fopenmp -o project2 project2.c

elif [ "$MODE" == "mpi" ]; then
    # Pure MPI mode
    mpicxx -D_MPI -o project2 project2.c

elif [ "$MODE" == "hybrid" ]; then
    # MPI + OpenMP hybrid mode
    mpicxx -fopenmp -D_MPI -o project2 project2.c

else
    echo "Usage: [mode] [size] [percent] [nProcesses | nThreads(MPI disabled)] [nThreads(MPI enabled)]"
    exit 1
fi

# TODO: To run experiment 1 (matrix size finding), uncomment the while loop and last 4 lines of code)
# while true; do
  # Execute based on the mode, reusing the compiled binary
if [ "$MODE" == "seq" ]; then
    # Sequential mode
    srun --time=00:10:00 ./project2 $SIZE $PERCENT

elif [ "$MODE" == "openmp" ]; then
    # Pure OpenMP mode
    srun --cpus-per-task=$ARG3 ./project2 $SIZE $PERCENT $ARG3

elif [ "$MODE" == "mpi" ]; then
    # Pure MPI mode
    srun --ntasks-per-node=$ARG3 ./project2 $SIZE $PERCENT

elif [ "$MODE" == "hybrid" ]; then
    # MPI + OpenMP hybrid mode
    srun --ntasks-per-node=$ARG3 --cpus-per-task=$ARG4 ./project2 $SIZE $PERCENT $ARG4
fi

# If srun exited due to timeout or error
if [ $? -ne 0 ]; then
    echo "[SBATCH] Srun terminated due to exceeding the time limit or error"
    # break
fi

#   # Increase matrix size by increment amount
#   SIZE=$(($SIZE + $INCREMENT))
#   echo "[SBATCH] Increasing matrix size to $SIZE"
# done
