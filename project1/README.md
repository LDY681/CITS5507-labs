# How to Run

## Experiment 1

### Generate Matrices with a Specific Non-Zero Density
> To generate matrices with a predefined non-zero density:

Use the following command to create matrices with 1% non-zero elements:
```bash
sbatch project1.sh init 1
```

### Run Matrix Multiplication for a Specific Thread Range and Matrix Size
> To perform matrix multiplication across a range of threads on a specific set of matrices:

This command runs the experiment using thread counts from 11 to 20 on matrices with 1% non-zero elements:
```bash
sbatch project1.sh start 11-20 1
```

## Experiment 2

### Run Scheduling Strategy Experiments
> To test and compare all four thread scheduling strategies:

Use this command to run the experiment with 60 threads on a matrix with 1% non-zero elements:
```bash
sbatch project1-p2.sh start 60 1
```
