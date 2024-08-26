#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <string>

// The dimension size of the matrix should be 100000 x 100000
#define NROWS = 100000
#define NCOLS = 100000


typedef struct {
    int nRows;        // Number of rows in the matrix
    int nCols;        // Number of columns in the matrix
    int nValues;         // Number of non-zero elements
    int *values;     // Array to store non-zero values
    int *colIndex;   // Array to store column indices of the non-zero values
    int *rowPtr;     // Array to store the index in values[] where each row starts
} CSRMatrix;

int main(int argc, char *argv[]) {
  int numThreads = argv[1];
  std::cout << "Num of Threads: " << numThreads << "\n";
  omp_set_num_threads(numThreads);


    // Allocate memory for sparse matrices
    int *Xvalues = malloc(NROWS * NCOLS * sizeof(int));
    int *XrowPtr = malloc((NROWS + 1) * sizeof(int));
    int *XcolIndex = malloc(NROWS * NCOLS * sizeof(int));

    int *Yvalues = malloc(NROWS * NCOLS * sizeof(int));
    int *YrowPtr = malloc((NROWS + 1) * sizeof(int));
    int *YcolIndex = malloc(NROWS * NCOLS * sizeof(int));

    int *result = malloc(NROWS * NCOLS * sizeof(int));
    
  // // Array of floats
  // int array_size = 276447232;
  // float *array = (float*)malloc(array_size * sizeof(float));
  // float arraySum = 0;

  // // initialize array and find the sum;
  // for (int i = 0; i < array_size; i++) {
  //     float item = ((float)rand() / RAND_MAX) * 1.0f;
  //     array[i] = item; // Numbers between 0 and 1
  //     arraySum += item;
  // }

  // // start clock on execution
  // clock_t start_time = clock();

  // float totalSum;
  // float localSum;
  // #pragma omp parallel private(localSum) reduction(+:totalSum)
  // {
  //   #pragma omp for
  //     for (int i = 0; i < array_size; i++) {
  //         localSum += array[i];
  //     }
  //   //Code here will be executed by all threads
  //   printf("localSum %f in thread %d\n", localSum, omp_get_thread_num());
  //   totalSum += localSum;
  //   // Make sure all threads see the most up-to-date totalSum
  //   #pragma omp flush(totalSum)
  // }
  // printf("totalSum in all threads %f\n Actual Sum of the array %f\n", totalSum, arraySum);



  // // Record the end time
  // clock_t end_time = clock();

  // // Calculate the elapsed time in seconds
  // double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
  // // Print the elapsed time
  // printf("Time taken to complete sum task: %f seconds\n", time_spent);
  // return 0;
}

/**
 * generateMatrix
 * @ generate the matrix with certain density/probability of non-zero values
 * @param {double} probability
 */
void generateMatrix(int *values, int *rowPtr, int *colIndex, double probability) {
    int count = 0;
    rowPtr[0] = 0;
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOLS; j++) {
            if ((double)rand() / RAND_MAX < probability) {
                values[count] = rand() % 10 + 1;  // Random value between 1 and 10
                colIndex[count] = j;
                count++;
            }
        }
        rowPtr[i + 1] = count;
    }
}

// Run with different threads



#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#define NROWS 100000
#define NCOLS 100000

// Function to generate a sparse matrix
void generateSparseMatrix(std::vector<int> &values, std::vector<int> &rowPtr, std::vector<int> &colIndex, double prob) {
    int count = 0;
    rowPtr.resize(NROWS + 1);
    rowPtr[0] = 0;
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOLS; j++) {
            if ((double)rand() / RAND_MAX < prob) {
                values.push_back(rand() % 10 + 1);  // Random value between 1 and 10
                colIndex.push_back(j);
                count++;
            }
        }
        rowPtr[i + 1] = count;
    }
}

// Function for ordinary matrix multiplication
void matrixMultiply(const std::vector<int> &Xvalues, const std::vector<int> &XrowPtr, const std::vector<int> &XcolIndex,
                    const std::vector<int> &Yvalues, const std::vector<int> &YrowPtr, const std::vector<int> &YcolIndex,
                    std::vector<int> &result) {
    result.resize(NROWS * NCOLS, 0);  // Initialize result with zeros
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NROWS; j++) {
            int sum = 0;
            int xStart = XrowPtr[i];
            int xEnd = XrowPtr[i + 1];
            int yStart = YrowPtr[j];
            int yEnd = YrowPtr[j + 1];
            for (int xi = xStart; xi < xEnd; xi++) {
                for (int yi = yStart; yi < yEnd; yi++) {
                    if (XcolIndex[xi] == YcolIndex[yi]) {
                        sum += Xvalues[xi] * Yvalues[yi];
                    }
                }
            }
            result[i * NCOLS + j] = sum;
        }
    }
}

int main() {
    srand(time(NULL));

    // Define the probabilities
    double probs[3] = {0.01, 0.02, 0.05};

    // Vectors to store CRS format matrices
    std::vector<int> Xvalues, XrowPtr, XcolIndex;
    std::vector<int> Yvalues, YrowPtr, YcolIndex;
    std::vector<int> result;

    for (int k = 0; k < 3; k++) {
        // Clear vectors for reuse
        Xvalues.clear();
        XrowPtr.clear();
        XcolIndex.clear();
        Yvalues.clear();
        YrowPtr.clear();
        YcolIndex.clear();
        result.clear();

        // Generate X and Y matrices
        generateSparseMatrix(Xvalues, XrowPtr, XcolIndex, probs[k]);
        generateSparseMatrix(Yvalues, YrowPtr, YcolIndex, probs[k]);

        // Multiply X and Y
        matrixMultiply(Xvalues, XrowPtr, XcolIndex, Yvalues, YrowPtr, YcolIndex, result);

        // Print a small part of the result for verification
        std::cout << "Result matrix for probability " << probs[k] << ":\n";
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                std::cout << result[i * NCOLS + j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}

