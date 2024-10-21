#!/bin/bash
#SBATCH --mem=100G
#SBATCH --time=04:00:00
#SBATCH --partition=work
#SBATCH --account=courses0101

# Arguments
NODES=$1        # Number of nodes
MODE=$2         # Mode: seq, openmp, mpi, or hybrid
SIZE=$3         # Starting Matrix size (SIZE x SIZE)
PERCENT=$4      # Matrix percentage
ARG3=$5         # Threads (OpenMP) or Processes (MPI)
ARG4=$6         # Threads (OpenMP for hybrid)

# Set the number of nodes based on user input
#SBATCH --nodes=$NODES
# Set OpenMP environment variables for optimal thread binding
export OMP_PROC_BIND=spread  # Ensure threads are spread across cores
export OMP_PLACES=cores      # Bind each thread to a specific core

# For small increment size
# INCREMENT=$(echo "$SIZE * 0.1" | bc)

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
    echo "Usage: [nNodes] [mode] [size] [percent] [nProcesses | nThreads(MPI disabled)] [nThreads(MPI enabled)]"
    exit 1
fi

# Run experiments with increasing size until timeout
while true; do
    LOGFILE="node_${NODES}_mode_${MODE}_size_${SIZE}_percent_${PERCENT}"
  # Execute based on the mode, reusing the compiled binary
  if [ "$MODE" == "seq" ]; then
      # Sequential mode
      LOGFILE+=".log"
      srun --time=00:10:00 ./project2 $SIZE $PERCENT > "$LOGFILE"

  elif [ "$MODE" == "openmp" ]; then
      # Pure OpenMP mode
      LOGFILE+="_thread_${ARG3}.log"
      srun --time=00:10:00 --cpus-per-task=$ARG3 ./project2 $SIZE $PERCENT $ARG3 > "$LOGFILE"

  elif [ "$MODE" == "mpi" ]; then
      # Pure MPI mode
      LOGFILE+="_process_${ARG3}.log"
      srun --time=00:10:00 --ntasks-per-node=$ARG3 ./project2 $SIZE $PERCENT > "$LOGFILE"

  elif [ "$MODE" == "hybrid" ]; then
      # MPI + OpenMP hybrid mode
      LOGFILE+="_process_${ARG3}_thread_${ARG4}.log"
      srun --time=00:10:00 --ntasks-per-node=$ARG3 --cpus-per-task=$ARG4 ./project2 $SIZE $PERCENT $ARG4 > "$LOGFILE"
  fi

  # If srun exited due to timeout or error
  if [ $? -ne 0 ]; then
    echo "Srun terminated due to exceeding the time limit or error"
    break
  fi

  # Increase matrix size by 10% of the original size
  #   SIZE=$(echo "$SIZE + $INCREMENT" | bc)
  #   SIZE=${SIZE%.*}  # Convert to integer by truncating decimal part

  # SIZE doubles for quick lookups
  SIZE=$(($SIZE * 2))
  echo "Increasing matrix size to $SIZE"
done