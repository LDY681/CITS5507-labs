#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=4G
#SBATCH --time=00:00:01
cc -o lab3 -fopenmp ./lab3.c
export OMP_NUM_THREADS=100
srun ./lab3
