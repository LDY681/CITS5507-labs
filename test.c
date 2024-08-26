#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
using namespace std;

// TODO try with different threads
#define NROWS 10
#define NCOLS 10

/**
 * generateMatrices
 * @description generate the mother matrix and two baby matrices with certain probability of non-zero values
 * @description vector are passed by reference e.g. &values to edit directly
 * @param {int} percent, probability of non-zeros
 * @param {vector<vector>>} original the uncompressed original matrix
 * @param {vector<vector>>} values the compressed value matrix
 * @param {vector<vector>>} indices the compressed index matrix
 */
void generateMatrices(vector<vector<int>> &original, vector<vector<int>> &values, vector<vector<int>> &indices, int percent) {
    cout << "Generating matrix:\n";
    for (int row = 0; row < NROWS; row++) {
        // 1d vector for uncompressed/values/indices each row
        vector <int> rowOriginal, rowValues, rowIndices;
        for (int col = 0; col < NCOLS; col++) {
            // if this element falls to non-zero jackpot 
            if (rand() % 100 < percent) {
                // Random value between 1 and 10
                int randValue = rand() % 10 + 1;

                // push both its value and index to row vectors.
                rowValues.push_back(randValue);
                rowIndices.push_back(col);
                rowOriginal.push_back(randValue);
                cout << randValue << " ";
            } else {
                // add zero value to original row
                rowOriginal.push_back(0);
                cout << 0 << " ";
            }
        }
        // edge case: if no non-zeros in a row, add consecutive zeros to fill up
        if (rowIndices.size() == 0) {
            rowValues.assign(NCOLS, 0);
            rowIndices.assign(NCOLS, 0);
        }
        original.push_back(rowOriginal);
        values.push_back(rowValues);
        indices.push_back(rowIndices);
        cout << endl;
    }
}

/**
 * matrixMultiply
 * @description The oridinary matrix multiply function on uncompressed matrices
 * @param {vector<vector>>} the X matrix to be multiplied
 * @param {vector<vector>>} the Y matrix
 * @return {vector<vector>>} the resulting matrix
 */
vector<vector<int>> matrixMultiply(vector<vector<int>> X, vector<vector<int>> Y) {
    // Initialize resulting matrix with all zeros
    vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0));

    cout << "Ordinary matrixMultiply:\n";
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOLS; j++) {
            for (int k = 0; k < NCOLS; k++) {
                result[i][j] += X[i][k] * Y[k][j];
            }
            cout << result[i][j] << " ";
        }
        cout << endl;
    }
    return result;
}

int main() {
    //! Three scenarios with different non-zero probabilities
    int percents[3] = {10, 20, 50};

    // Matrix X = Xindices * Xvalues
    // Matrix Y = Yindices * Yvalues
    vector<vector<int>> X, Y;
    vector<vector<int>> Xindices, Xvalues;
    vector<vector<int>> Yindices, Yvalues;
    vector<vector<int>> outputs;
    for (int i = 0; i < 3; i++) {
        // Clear vectors for reuse
        X.clear();
        Xindices.clear();
        Xvalues.clear();
        Y.clear();
        Yindices.clear();
        Yvalues.clear();

        // Generate X and Y values/indices matrices
        generateMatrices(X, Xvalues, Xindices, percents[i]);
        generateMatrices(Y, Yvalues, Yindices, percents[i]);

        // Multiply X and Y
        matrixMultiply(X, Y);
    }
    return 0;
}
