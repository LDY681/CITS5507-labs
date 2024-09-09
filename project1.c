    #include <omp.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <iostream>
    #include <string>
    #include <vector>
    #include <chrono>
    using namespace std;

    #define DEBUG false // Enable to output matrix generation and check integrity
    #define NROWS 100000 // Number of rows of the matrix
    #define NCOLS 100000 // Number of columns of the matrix

    /**
     * generateMatrices
     * @description generate the mother matrix and two baby matrices with certain probability of non-zero values
     * @description vector are passed by reference e.g. &values to edit directly
     * @param original {vector<vector>>} the uncompressed original matrix
     * @param values {vector<vector>>} the compressed value matrix
     * @param indices {vector<vector>>} the compressed index matrix
     * @param percent {int}, probability of non-zeros
     * @param suffix {string}, suffix for the fileName to distinguish matrix X and matrix Y
     */
    void generateMatrices(vector<vector<int>> &original, vector<vector<int>> &values, vector<vector<int>> &indices, int percent, string suffix) {
        if (DEBUG) cout << "Generating matrix:\n";

        // open two files for writing
        FILE *fpb, *fpc;
        string fileB = "FileB_matrix" + suffix + "_percent_" + to_string(percent);
        string fileC = "FileC_matrix" + suffix + "_percent_" + to_string(percent);
        fpb=fopen(fileB.c_str(), "w");
        fpc=fopen(fileC.c_str(), "w");

        if (fpb == nullptr || fpc == nullptr) {
            cerr << "Error opening files!" << endl;
            return;
        }

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
                    
                    // write value and index into file
                    fprintf(fpb," %d", randValue);
                    fprintf(fpc," %d", col);
                    if (DEBUG) cout << randValue << " ";
                } else {
                    // add zero value to original row
                    rowOriginal.push_back(0);
                    if (DEBUG) cout << 0 << " ";
                }
            }
            // edge case: if no non-zeros in a row, fill 2-consecutive zeros on position 0-1 for indication
            if (rowIndices.size() == 0) {
                rowValues.assign(2, 0);
                rowIndices.assign(2, 0);
                
                // write value and index into file
                fprintf(fpb," %d %d", 0, 0);
                fprintf(fpc," %d %d", 0, 0);
            }
            original.push_back(rowOriginal);
            values.push_back(rowValues);
            indices.push_back(rowIndices);

            // write endl into file
            fprintf(fpb, "\n");
            fprintf(fpc, "\n");
            if (DEBUG) cout << endl;
        }

        // Close the files after writing
        fclose(fpb);
        fclose(fpc);
    }


    void loadMatrices(vector<vector<int>> &values, vector<vector<int>> &indices, int percent, string suffix) {
        string fileB = "FileB_matrix" + suffix + "_percent_" + to_string(percent);
        string fileC = "FileC_matrix" + suffix + "_percent_" + to_string(percent);

        // Open the files for reading
        FILE *fpb = fopen(fileB.c_str(), "r");
        FILE *fpc = fopen(fileC.c_str(), "r");

        if (fpb == nullptr || fpc == nullptr) {
            cerr << "Error opening files!" << endl;
            return;
        }

        // Read values from fileB and indices from fileC
        int value, index;
        int row = 0;
        while (!feof(fpb) && !feof(fpc)) {
            vector<int> rowValues;
            vector<int> rowIndices;

            // One row of values and indices
            while (fscanf(fpb, "%d", &value) == 1 && fscanf(fpc, "%d", &index) == 1) {
                if (DEBUG) {
                    cout << "matrix: " << suffix << " row: " << row << " value: " << value << " index: " << index << endl;
                }
                rowValues.push_back(value);
                rowIndices.push_back(index);

                // Check if we've reached the end of the row
                if (fgetc(fpb) == '\n' || fgetc(fpc) == '\n') {
                    break;
                }
            }

            values.push_back(rowValues);
            indices.push_back(rowIndices);
            row++;
        }

        // Close the files after reading
        fclose(fpb);
        fclose(fpc);
    }

    /**
     * matrixMultiply
     * @description The oridinary matrix multiply function on uncompressed matrices
     * @param X {vector<vector<>>} the X matrix to be multiplied
     * @param Y {vector<vector<>>} the Y matrix
     * @return {vector<vector<>>} the resulting matrix
     */
    vector<vector<int>> matrixMultiply(vector<vector<int>> &X, vector<vector<int>> &Y) {
        // Initialize resulting matrix with all zeros
        vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0));

        if (DEBUG) cout << "Ordinary matrixMultiply:\n";
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


    /**
     * compressedMatrixMultiply
     * @description The matrix multiply function on compressed matrices
     * @param Xvalues {vector<vector<>>} the Xvalues matrix
     * @param Xindices {vector<vector<>>} the Xindices matrix
     * @param Yvalues {vector<vector<>>} the Yvalues matrix
     * @param Yindices {vector<vector<>>} the Yindices matrix
     * @param NThreads {int} number of threads
     */
    vector<vector<int>> compressedMatrixMultiply(vector<vector<int>> &Xvalues, vector<vector<int>> &Xindices, vector<vector<int>> &Yvalues, vector<vector<int>> &Yindices, int NThreads) {
        // Initialize resulting matrix with all zeros
        vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0));

        if (DEBUG) cout << "Compressed matrixMultiply:\n";
        #pragma omp parallel for num_threads(NThreads)
        for (int i = 0; i < Xvalues.size(); i++) {   // # of rows are fixed
            for (int j = 0; j < Xvalues[i].size(); j++) {
                int X_value = Xvalues[i][j];
                int X_indice = Xindices[i][j];

                // Multiply row of X with corresponding column of Y
                for (int k = 0; k < Yvalues[X_indice].size(); k++) {
                    int Y_value = Yvalues[X_indice][k];
                    int Y_indice = Yindices[X_indice][k];

                    //! TODO this operation creates lots of overhead, but creating local copies of huge matrix is impractical....
                    #pragma omp atomic
                    result[i][Y_indice] += X_value * Y_value;
                }
            }
        }

        if (DEBUG) {
            for (int row = 0; row < NROWS; row++) {
                for (int col = 0; col < NCOLS; col++) {
                    cout << result[row][col] << " ";
                }
                cout << endl;
            }
        }
        return result;
    }

    /**
     * checkIntegrity
     * @description check if compressedMatrixMultiply has the same output as ordinary matrixMultiply
     */
    bool checkIntegrity(vector<vector<int>> source, vector<vector<int>> target) {
        if (source.size() != target.size()) {
            return false;
        }

        for (int i = 0; i < source.size(); i++) {
            if (source[i].size() != target[i].size()) {
                return false;
            }

            for (int j = 0; j < source[i].size(); j++) {
                if (source[i][j] != target[i][j]) {  
                    return false;
                }
            }
        }

        // If all elements are the same
        return true;
    }

    int main(int argc, char *argv[]) {
        int percent = 0, minThreads = 0, maxThreads = 0;
        if (argc < 3) {
            cout << "Usage: %s [init | start] [percent | thread_range] [percent (if mode set to start)]\n" << endl;
            return 1;
        }
        string mode = argv[1];
        string param1 = argv[2];
        string param2 = argc > 3 ? argv[3] : "";
        if (mode == "start") {
            // thread range
            minThreads = stoi(param1.substr(0, param1.find('-')));
            maxThreads = stoi(param1.substr(param1.find('-') + 1));
            // percent
            percent = stoi(param2);
        } else {
            // percent
            percent = stoi(param1);
        }

        // Parameter checks
        cout << "==================Running Project====================" << endl;
        cout << "DEBUG: " << (DEBUG ? "true" : "false") << endl;
        cout << "mode: " << mode << endl;
        if (mode == "init") {
            cout << "percent: " << percent << endl;
        } else {
            cout << "percent: " << percent << endl;
            cout << "minThreads: " << minThreads << endl;
            cout << "maxThreads: " << maxThreads << endl;
        }
        cout << "NROWS: " << NROWS << endl;
        cout << "NCOLS: " << NCOLS << endl;

        // Matrix X = Xindices * Xvalues
        // Matrix Y = Yindices * Yvalues
        vector<vector<int>> X, Y;
        vector<vector<int>> Xindices, Xvalues;
        vector<vector<int>> Yindices, Yvalues;
        vector<vector<int>> outputOriginal, outputCompressed;

        if (mode == "init") {
            // Generate three pairs of matrices with different probability
            cout << "==================Generating Matrices====================" << endl;
            cout << "Generating matrices with probability: " << percent << endl;
            generateMatrices(X, Xvalues, Xindices, percent, "X");
            generateMatrices(Y, Yvalues, Yindices, percent, "Y");

            // Compres ordinary matrix multiply and compressed matrix multiply
            if (DEBUG) outputOriginal = matrixMultiply(X, Y);
            if (DEBUG) outputCompressed = compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, 256);
            if (DEBUG) cout << "Are these two matrix identical?: " << boolalpha << checkIntegrity(outputOriginal, outputCompressed) << endl;

            cout << "Matrices generated!" << endl;
        } else {
            cout << "==================Loading Matrices====================" << endl;
            cout << "Loading matrices with probability: " << percent << endl;
            loadMatrices(Xvalues, Xindices, percent, "X");
            loadMatrices(Yvalues, Yindices, percent, "Y");
            cout << "Matrices loaded!" << endl;
        }

        // Experiement with different threads
        if (mode == "start") {
            cout << "==================Starting Experiments====================" << endl;
            for (int num_threads = minThreads; num_threads <= maxThreads; num_threads++) {
                cout << "<<<<<<<<<< Evaluating timelapse with probability: " << percent << " and " << num_threads << " threads >>>>>>>>>>" << endl;

                // Time counter + compressed matrix multiplication
                auto start = chrono::high_resolution_clock::now();
                compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, num_threads);
                auto end = chrono::high_resolution_clock::now();

                // Print timelapse
                chrono::duration<double> elapsed = end - start;
                time_t end_time = chrono::system_clock::to_time_t(end);
                cout << "Finished at " << ctime(&end_time) << "Elapsed time: " << elapsed.count() << "s\n";
            }
        }
        return 0;
    }
