#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _MPI
#include <mpi.h>
#endif
using namespace std;

#define DEBUG false // Enable to output matrix generation and matrix multiplication
int NROWS = 10000; // Number of rows of the matrix
int NCOLS = 10000; // Number of columns of the matrix

// Function to write matrices to files in rank 0 (for debug mode)
void writeMatrixToFile(const vector<vector<int>> &values, const vector<vector<int>> &indices, string suffix) {

    // open two files for writing
    FILE *fpb, *fpc;
    string fileB = "FileB_matrix" + suffix;
    string fileC = "FileC_matrix" + suffix;

    fpb=fopen(fileB.c_str(), "w");
    fpc=fopen(fileC.c_str(), "w");

    if (fpb == nullptr || fpc == nullptr) {
        cerr << "Error opening files!" << endl;
        return;
    }

    for (int i = 0; i < values.size(); i++) {
        for (int j = 0; j < values[i].size(); j++) {
            // write value and index into file
            fprintf(fpb, "%d ", values[i][j]);
            fprintf(fpc, "%d ", indices[i][j]);
        }

        // write endl into file
        fprintf(fpb, "\n");
        fprintf(fpc, "\n");
    }

    // Close the files after writing
    fclose(fpb);
    fclose(fpc);
}

/**
 * generateMatrices
 * @description generate two baby matrices with certain probability of non-zero values
 * @description Each MPI process will generate part of the matrices
 * @param values {vector<vector>>} the compressed value matrix
 * @param indices {vector<vector>>} the compressed index matrix
 * @param percent {int} probability of non-zeros
 * @param rank {int} MPI rank for partitioning
 * @param nProcesses {int} Number of MPI processes
 */
void generateMatrices(vector<vector<int>>& values, vector<vector<int>>& indices, int percent, int rank, int nProcesses) {
    for (int row = 0; row < NROWS; row++) {
        vector<int> rowValues, rowIndices;
        for (int col = 0; col < NCOLS; col++) {
            if (rand() % 100 < percent) {
                // Random value between 1 and 10
                int randValue = rand() % 10 + 1;

                // push both its value and index to row vectors.
                rowValues.push_back(randValue);
                rowIndices.push_back(col);
            }
        }
        // edge case: if no non-zeros in a row, fill 2-consecutive zeros on position 0-1 for indication
        if (rowIndices.size() == 0) {
            rowValues.assign(2, 0);
            rowIndices.assign(2, 0);
        }
        values.push_back(rowValues);
        indices.push_back(rowIndices);
    }
}

/**
 * compressedMatrixMultiply
 * @description The matrix multiply function on compressed matrices
 * @param Xvalues {vector<vector<>>} the Xvalues matrix
 * @param Xindices {vector<vector<>>} the Xindices matrix
 * @param Yvalues {vector<vector<>>} the Yvalues matrix
 * @param Yindices {vector<vector<>>} the Yindices matrix
 * @param result {vector<vector<int>>} the resulting matrix
 * @param rank {int} MPI rank for partitioning
 * @param nProcesses {int} Number of MPI processes
 */
void compressedMatrixMultiply(const vector<vector<int>>& Xvalues, const vector<vector<int>>& Xindices, 
                              const vector<vector<int>>& Yvalues, const vector<vector<int>>& Yindices,
                              vector<vector<int>>& result, int rank, int nProcesses) {

    int local_rows = NROWS / nProcesses;
    int start_row = rank * local_rows;
    int end_row = (rank == nProcesses - 1) ? NROWS : start_row + local_rows;
    
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < Xvalues[i].size(); j++) {
            int X_value = Xvalues[i][j];
            int X_indice = Xindices[i][j];
            for (int k = 0; k < Yvalues[X_indice].size(); ++k) {
                int Y_value = Yvalues[X_indice][k];
                int Y_indice = Yindices[X_indice][k];
#ifdef _OPENMP
                //! this operation creates lots of overhead, but creating local copies of huge matrix is impractical....
                #pragma omp atomic
#endif
                result[i][Y_indice] += X_value * Y_value;
            }
        }
    }
#ifdef _MPI
    // Synchronize processes after computation
    MPI_Barrier(MPI_COMM_WORLD);

    // Gather the results from all processes to rank 0
    if (rank == 0) {
        for (int i = 1; i < nProcesses; ++i) {
            MPI_Recv(&result[0][0], NROWS * NCOLS, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        // Send local results to rank 0
        MPI_Send(&result[start_row][0], local_rows * NCOLS, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
#endif
}

// Flatten compressed matrix and broadcast row sizes first
void flattenCompressedMatrix(const vector<vector<int>>& matrix, vector<int>& flattened, vector<int>& rowSizes) {
    for (const auto& row : matrix) {
        rowSizes.push_back(row.size());
        flattened.insert(flattened.end(), row.begin(), row.end());
    }
}

// Reconstruct compressed matrix based on row sizes
void reconstructCompressedMatrix(vector<vector<int>>& matrix, const vector<int>& flattened, const vector<int>& rowSizes) {
    int idx = 0;
    for (int i = 0; i < matrix.size(); ++i) {
        matrix[i].assign(flattened.begin() + idx, flattened.begin() + idx + rowSizes[i]);
        idx += rowSizes[i];
    }
}

/**
 * startExperiment
 * @description Time the entire experiment, and write matrices if in debug mode
 * @param rank {int}, MPI rank
 * @param nProcesses {int}, Number of MPI processes
 * @param nThreads {int}, Number of OpenMP threads
 * @param percent {int}, Density of non-zero elements
 */
void startExperiment(int rank, int nProcesses, int nThreads, int percent) {
    vector<vector<int>> Xvalues, Xindices;
    vector<vector<int>> Yvalues, Yindices;
    vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0)); // Resulting matrix

    // Flatten matrices for broadcast messaging sending
    vector<int> flattened_Xvalues, flattened_Xindices;
    vector<int> flattened_Yvalues, flattened_Yindices;

    // rowSizes Information for flattened matrices
    vector<int> rowSizes_Xvalues, rowSizes_Xindices;
    vector<int> rowSizes_Yvalues, rowSizes_Yindices;

    if (rank == 0) {
        cout << "==================Generating Matrices====================" << endl;
        // Generate compressed matrices with target matrix size and density
        generateMatrices(Xvalues, Xindices, percent, rank, nProcesses);
        generateMatrices(Yvalues, Yindices, percent, rank, nProcesses);
        cout << "==================Mutiplying Matrices====================" << endl;

        flattenCompressedMatrix(Xvalues, flattened_Xvalues, rowSizes_Xvalues);
        flattenCompressedMatrix(Xindices, flattened_Xindices, rowSizes_Xindices);
        flattenCompressedMatrix(Yvalues, flattened_Yvalues, rowSizes_Yvalues);
        flattenCompressedMatrix(Yindices, flattened_Yindices, rowSizes_Yindices);
    }

    // Broadcast generated matrices to all processes
    // 1. Broadcast row sizes first
    if (rank != 0) {
        rowSizes_Xvalues.resize(NROWS);
        rowSizes_Xindices.resize(NROWS);
        rowSizes_Yvalues.resize(NROWS);
        rowSizes_Yindices.resize(NROWS);
    }

    MPI_Bcast(rowSizes_Xvalues.data(), NROWS, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(rowSizes_Xindices.data(), NROWS, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(rowSizes_Yvalues.data(), NROWS, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(rowSizes_Yindices.data(), NROWS, MPI_INT, 0, MPI_COMM_WORLD);

    // Resize matrices to receive broadcast data
    if (rank != 0) {
        Xvalues.resize(NROWS);
        Xindices.resize(NROWS);
        Yvalues.resize(NROWS);
        Yindices.resize(NROWS);
    }

    // Calculate the total size of the flattened matrices for each process
    int flattened_Xvalues_size = 0;
    int flattened_Xindices_size = 0;
    int flattened_Yvalues_size = 0;
    int flattened_Yindices_size = 0;

    for (int i = 0; i < NROWS; ++i) {
        flattened_Xvalues_size += rowSizes_Xvalues[i];
        flattened_Xindices_size += rowSizes_Xindices[i];
        flattened_Yvalues_size += rowSizes_Yvalues[i];
        flattened_Yindices_size += rowSizes_Yindices[i];
    }

    // Resize flattened matrices on non-rank 0 processes
    if (rank != 0) {
        flattened_Xvalues.resize(flattened_Xvalues_size);
        flattened_Xindices.resize(flattened_Xindices_size);
        flattened_Yvalues.resize(flattened_Yvalues_size);
        flattened_Yindices.resize(flattened_Yindices_size);
    }

    // 2. Broadcast flattened matrices
    MPI_Bcast(flattened_Xvalues.data(), flattened_Xvalues.size(), MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(flattened_Xindices.data(), flattened_Xindices.size(), MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(flattened_Yvalues.data(), flattened_Yvalues.size(), MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(flattened_Yindices.data(), flattened_Yindices.size(), MPI_INT, 0, MPI_COMM_WORLD);

    // Reconstruct compressed matrices based on rowSizes
    if (rank != 0) {
        reconstructCompressedMatrix(Xvalues, flattened_Xvalues, rowSizes_Xvalues);
        reconstructCompressedMatrix(Xindices, flattened_Xindices, rowSizes_Xindices);
        reconstructCompressedMatrix(Yvalues, flattened_Yvalues, rowSizes_Yvalues);
        reconstructCompressedMatrix(Yindices, flattened_Yindices, rowSizes_Yindices);
    }

#ifdef _OPENMP
    omp_set_num_threads(nThreads);
#endif

    auto start = std::chrono::high_resolution_clock::now();
    // Matrix multiplication
    compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, result, rank, nProcesses);

    // Synchronize before time measurement
#ifdef _MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    auto end = std::chrono::high_resolution_clock::now();
    time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::chrono::duration<double> elapsed_time = end - start;
    double elapsed = elapsed_time.count();

    // Only rank 0 outputs the results
    if (rank == 0) {
        cout << "Finished at " << ctime(&end_time) << "Elapsed time: " << elapsed << "s\n";

        // If debug mode, write matrices to files
        if (DEBUG) {
            string suffix = "_size_" + to_string(NROWS) + "_percent_" + to_string(percent);
            writeMatrixToFile(Xvalues, Xindices, "X" + suffix);
            writeMatrixToFile(Yvalues, Yindices, "Y" + suffix);
            writeMatrixToFile(result, result, "XY" + suffix);
        }
    }
}

/**
 * broadcastMatrix
 * @description Broadcast matrices to all MPI processes
 */
void broadcastMatrix(vector<vector<int>>& values, vector<vector<int>>& indices, int rank) {
    // Flatten the matrix so we can sent MPI broadcast
    int value_size = values.size() * NCOLS;
    int index_size = indices.size() * NCOLS;

    vector<int> flat_values(value_size);
    vector<int> flat_indices(index_size);

    if (rank == 0) {
        for (int i = 0; i < values.size(); i++) {
            copy(values[i].begin(), values[i].end(), flat_values.begin() + i * NCOLS);
            copy(indices[i].begin(), indices[i].end(), flat_indices.begin() + i * NCOLS);
        }
    }

    // Broadcast the sizes of the flattened vectors
    MPI_Bcast(&value_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&index_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast the flattened vectors
    MPI_Bcast(flat_values.data(), value_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(flat_indices.data(), index_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Deflatten array
    if (rank != 0) {
        values.resize(NROWS);
        indices.resize(NROWS);
        for (int i = 0; i < NROWS; i++) {
            values[i].assign(flat_values.begin() + i * NCOLS, flat_values.begin() + (i + 1) * NCOLS);
            indices[i].assign(flat_indices.begin() + i * NCOLS, flat_indices.begin() + (i + 1) * NCOLS);
        }
    }
}

int main(int argc, char *argv[]) {
    int nSize = 10000; // Default matrix size
    int percent = 1;  // Matrix density in percentage
    int rank = 0;  // MPI current process
    int nProcesses = 1;  // MPI processes
    int nThreads = 1;  // OpenMP threads

#ifdef _MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN); // Error handling
#endif

    // Check command-line arguments
   if (argc < 3) {
        cout << "Usage: %s [nSize] [percent] [nThreads(OpenMP enabled)] \n" << endl;
        return 1;
    }
    if (argc > 1) {
        nSize = atoi(argv[1]);
        NROWS = NCOLS = nSize;
    }
    if (argc > 2) percent = atoi(argv[2]);
    if (argc > 3) nThreads = atoi(argv[3]);

    // Print matrix setup
    if (rank == 0) {
        cout << "==================Running Project====================" << endl;
        cout << "NROWS: " << NROWS << " NCOLS: " << NCOLS << " Percent: " << percent << endl;
    // Print MPI and OPENMP related information
#ifdef _OPENMP
    #ifdef _MPI
        // If both OpenMPI and MPI are enabled
        cout << "[MPI+OPENMP] Running MPI with " << nProcesses << " processes," << " OpenMP with " << nThreads << " threads." << endl;
    #else 
        // If only OpenMP is enabled
        cout << "[OEPNMP] Running OpenMP with " << nThreads << " threads." << endl;
    #endif
#else
    #ifdef _MPI
        // if only MPI is enabled
        cout << "[MPI] Running MPI with " << nProcesses << " processes." << endl;
    #else
        // if neither MPI nor OpenMP is enabled
        cout << "[SEQ] Running sequential with 1 processes and 1 threads." << endl;
    #endif
#endif
    }

    // // Start the experiment
    startExperiment(rank, nProcesses, nThreads, percent);

#ifdef _MPI
    MPI_Finalize();
#endif
    return 0;
}
