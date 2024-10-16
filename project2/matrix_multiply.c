#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef MPI_ENABLED
#include <mpi.h>
#endif
using namespace std;


int NROWS = 10000 // Number of rows of the matrix
int NCOLS = 10000 // Number of columns of the matrix

/**
 * generateMatrices
 * @description generate two baby matrices with certain probability of non-zero values
 * @description vector are passed by reference e.g. &values to edit directly
 * @param values {vector<vector>>} the compressed value matrix
 * @param indices {vector<vector>>} the compressed index matrix
 * @param percent {int}, probability of non-zeros
 */
void generateMatrices(vector<vector<int>>&values, vector<vector<int>>&indices, int size, int percent) {
    for (int row = 0; row < NROWS; row++) {
        // 1d vector for uncompressed/values/indices each row
        vector<int> rowValues, rowIndices;
        for (int col = 0; col < NCOLS; col++) {
            // if this element falls to non-zero jackpot 
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
 */
void compressedMatrixMultiply(const vector<vector<int>>& Xvalues, const vector<vector<int>>& Xindices, 
                              const vector<vector<int>>& Yvalues, const vector<vector<int>>& Yindices) {

// Initialize resulting matrix with all zeros
vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0));

#ifdef _OPENMP
    #pragma omp parallel
#endif
    for (int i = 0; i < Xvalues.size(); i++) {
        for (int j = 0; j < Xvalues[i].size(); j++) {   // # of rows are fixed
            int X_value = Xvalues[i][j];
            int X_indice = Xindices[i][j];

            // Multiply row of X with corresponding column of Y
            for (int k = 0; k < Yvalues[X_indice].size(); ++k) {
                int Y_value = Yvalues[X_indice][k];
                int Y_indice = Yindices[X_indice][k];
                
#ifdef _OPENMP
                //! this operation creates lots of overhead, but creating local copies of huge matrix is impractical....
                #pragma omp atomic
#endif
                C[i][Y_indice] += X_value * Y_value;
            }
        }
    }
    return result;
}

int main(int argc, char *argv[]) {
    int nSize = 10000;  // Matrix size
    int percent = 1;  // Matrix density
    int nThreads = 1;  // OpenMP threads
    int nProcesses = 1;  // MPI processes

#ifdef MPI_ENABLED
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
#endif

    // Check command-line arguments
    if (argc > 1) N = atoi(argv[1]);  // Matrix size
    if (argc > 2) density = atof(argv[2]);  // Density of non-zero elements
    if (argc > 3) num_threads = atoi(argv[3]);  // Number of threads for OpenMP
    if (argc > 4) num_processes = atoi(argv[4]);  // Number of processes for MPI

    if (num_processes > 1) {
        cout << "Running with MPI across " << num_processes << " processes." << endl;
    }

#ifdef _OPENMP
    if (num_threads > 1) {
        cout << "Running with OpenMP using " << num_threads << " threads." << endl;
    }
#endif

    // Initialize compressed sparse matrices
    vector<vector<int>> Xvalues, Xindices;
    vector<vector<int>> Yvalues, Yindices;
    vector<vector<int>> C(N, vector<int>(N, 0));  // Result matrix

    // Generate random compressed matrices
    generateMatrices(Xvalues, Xindices, N, density);
    generateMatrices(Yvalues, Yindices, N, density);

    // Time the multiplication
    auto start = chrono::high_resolution_clock::now();
    compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, C, N, num_threads);
    auto end = chrono::high_resolution_clock::now();

    // Output time taken
    chrono::duration<double> elapsed = end - start;
    cout << "Matrix multiplication took: " << elapsed.count() << " seconds" << endl;

#ifdef MPI_ENABLED
    MPI_Finalize();
#endif

    return 0;
}
