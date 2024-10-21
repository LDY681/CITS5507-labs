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
    int local_rows = NROWS / nProcesses;  // Partition rows among processes
    int start_row = rank * local_rows;
    int end_row = (rank == nProcesses - 1) ? NROWS : start_row + local_rows; // For last partition, use all remaining rows

    for (int row = start_row; row < end_row; row++) {
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
 */
void compressedMatrixMultiply(const vector<vector<int>>& Xvalues, const vector<vector<int>>& Xindices, 
                              const vector<vector<int>>& Yvalues, const vector<vector<int>>& Yindices,
                              vector<vector<int>>& result) {
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < Xvalues.size(); i++) {
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
    auto start = std::chrono::high_resolution_clock::now();

    cout << "==================Generating Matrices====================" << endl;
    // Generate compressed matrices with target matrix size and density
    vector<vector<int>> Xvalues, Xindices;
    vector<vector<int>> Yvalues, Yindices;
    vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0)); // Resulting matrix
    generateMatrices(Xvalues, Xindices, percent, rank, nProcesses);
    generateMatrices(Yvalues, Yindices, percent, rank, nProcesses);

    // Synchronize before starting matrix multiplication
#ifdef _MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    cout << "==================Mutiplying Matrices====================" << endl;

#ifdef _OPENMP
    omp_set_num_threads(nThreads);
#endif

    // Matrix multiplication
    compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, result);

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

    // Print matrix setup
    if (rank == 0) {
        cout << "==================Running Project====================" << endl;
        cout << "NROWS: " << NROWS << " NCOLS: " << NCOLS << " Percent: " << percent << endl;
    }

    // Print MPI and OPENMP related information
#ifdef _OPENMP
    if (argc > 3) nThreads = atoi(argv[3]);
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

    // Start the experiment
    startExperiment(rank, nProcesses, nThreads, percent);

#ifdef _MPI
    MPI_Finalize();
#endif
    return 0;
}