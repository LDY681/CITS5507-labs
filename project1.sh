#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --partition=work
#SBATCH --account=courses0101
#SBATCH --mem=4G
#SBATCH --time=00:00:01

# compile project1 executable
g++ -o project1 -fopenmp ./project1.c

# run executable for 28 times, with thread number 1-28
for x in {1..1}
do
    srun ./project1 $x
done