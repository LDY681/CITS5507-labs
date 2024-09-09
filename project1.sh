#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=220G
#SBATCH --time=23:59:59

# [init | start]
ARG1=$1
# [percent | thread_range]
ARG2=$2
# [percent (if mode set to start)
ARG3=$3

# extract the upper limit of the thread range for cpus-per-task
MAXTHREADS=$(echo $ARG2 | cut -d'-' -f2)

echo "srun ./project1 $ARG1 $ARG2 ${ARG3:-"No ARG3"}"
echo "Running with --cpus-per-task=$MAXTHREADS"

# set OpenMP environment variables for optimal thread binding
export OMP_PROC_BIND=spread  # Ensure threads are spread across cores
export OMP_PLACES=cores      # Bind each thread to a specific core

# compile project executable
g++ -o project1 -fopenmp ./project1.c

# adjust cpus-per-task based on max thread count
#SBATCH --cpus-per-task=$MAXTHREADS

# pass the probability for matrix generation
srun ./project1 $ARG1 $ARG2 $ARG3