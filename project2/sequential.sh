#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=100G
#SBATCH --time=00:10:00

# TODO
# up to 4 nodes per user
# --nodes=4 # of nodes

# up to 128 physical cores (threads? processes?) per node
# --ntasks-per-node=4 MPI processes per node
# --cpus-per-task=32 OpenMP threads per node

# Combinations
# 0.01, 0.02, 0.05
# sequential, openmp, mpi, openmp+mpi
# 4 nodes & 128 cores each node & 10 minutes

N=10000  # Start with a small matrix size
DENSITY=0.01  # Non-zero element density

# Compile the C++ code
g++ -o project2-matrix project2-matrix.cpp

while true; do
  # Run the matrix multiplication
  srun ./project2-matrix $N $DENSITY > output.log

  # Parse the output to get the execution time
  TIME=$(grep "Matrix multiplication took" output.log | awk '{print $4}')

  # Check if the time exceeds 600 seconds (10 minutes)
  if (( $(echo "$TIME > 600.0" | bc -l) )); then
    echo "Matrix size $N exceeds 10 minutes. Stopping."
    break
  fi

  # Increase matrix size for the next iteration
  N=$(($N * 2))
  echo "Increasing matrix size to $N"
done
