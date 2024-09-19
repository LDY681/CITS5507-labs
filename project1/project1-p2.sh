#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=220G
#SBATCH --time=23:59:59

# [start]
ARG1=$1
# [num_of_threads]
ARG2=$2
# [percent]
ARG3=$3

echo "srun ./project1 $ARG1 $ARG2 ${ARG3:-"No ARG3"}"
echo "Running with --cpus-per-task=$ARG2"

# set OpenMP environment variables for optimal thread binding
export OMP_PROC_BIND=spread  # Ensure threads are spread across cores
export OMP_PLACES=cores      # Bind each thread to a specific core

# compile project executable
g++ -o project1-p2 -fopenmp ./project1-p2.c

# pass the probability for matrix generation
srun --cpus-per-task=$ARG2 ./project1-p2 $ARG1 $ARG2 $ARG3