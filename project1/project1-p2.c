    #include <omp.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <iostream>
    #include <string>
    #include <vector>
    #include <chrono>
    using namespace std;

    #define NROWS 100000 // Number of rows of the matrix
    #define NCOLS 100000 // Number of columns of the matrix

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
     * compressedMatrixMultiply
     * @description The matrix multiply function on compressed matrices
     * @param Xvalues {vector<vector<>>} the Xvalues matrix
     * @param Xindices {vector<vector<>>} the Xindices matrix
     * @param Yvalues {vector<vector<>>} the Yvalues matrix
     * @param Yindices {vector<vector<>>} the Yindices matrix
     * @param scheduling {string} types of scheduling (dynamic, guided, runtime, static)
     * @param chunk_size {int} the size of the chunk for scheduling
     */
    vector<vector<int>> compressedMatrixMultiply(vector<vector<int>> &Xvalues, vector<vector<int>> &Xindices, vector<vector<int>> &Yvalues, vector<vector<int>> &Yindices, string scheduling, int chunk_size) {
        // Initialize resulting matrix with all zeros
        vector<vector<int>> result(NROWS, vector<int>(NCOLS, 0));

        // test different scheduling strategies (with default chunk size)
        if (scheduling == "dynamic") {
            omp_set_schedule(omp_sched_dynamic, chunk_size);
        } else if (scheduling == "guided") {
            omp_set_schedule(omp_sched_guided, chunk_size);
        } else if (scheduling == "runtime") {
            // No need to call set_schedule, use default environment variables
        } else {
            omp_set_schedule(omp_sched_static, chunk_size);
        }

        // schedule is already set by previous conditionals and now apply the changes with schedule(runtime)
        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < Xvalues.size(); i++) {   // # of rows are fixed
            for (int j = 0; j < Xvalues[i].size(); j++) {
                int X_value = Xvalues[i][j];
                int X_indice = Xindices[i][j];

                // Multiply row of X with corresponding column of Y
                for (int k = 0; k < Yvalues[X_indice].size(); k++) {
                    int Y_value = Yvalues[X_indice][k];
                    int Y_indice = Yindices[X_indice][k];

                    //! this operation creates lots of overhead, but creating local copies of huge matrix is impractical....
                    #pragma omp atomic
                    result[i][Y_indice] += X_value * Y_value;
                }
            }
        }
        return result;
    }

    int main(int argc, char *argv[]) {
        int percent = 0, num_threads = 0;
        if (argc < 3) {
            cout << "Usage: %s [start] [num_of_threads] [percent]\n" << endl;
            return 1;
        }
        string mode = argv[1];
        string param1 = argv[2];
        string param2 = argv[3];
        if (mode == "start") {
            // thread range
            num_threads = stoi(param1);
            // percent
            percent = stoi(param2);
        }

        // Parameter checks
        cout << "==================Running Project Part 2====================" << endl;
        cout << "mode: " << mode << endl;
        cout << "num_of_threads: " << num_threads << endl;
        cout << "percent: " << percent << endl;
        cout << "NROWS: " << NROWS << endl;
        cout << "NCOLS: " << NCOLS << endl;

        // Matrix X = Xindices * Xvalues
        // Matrix Y = Yindices * Yvalues
        vector<vector<int>> X, Y;
        vector<vector<int>> Xindices, Xvalues;
        vector<vector<int>> Yindices, Yvalues;
        vector<vector<int>> outputCompressed;

        cout << "==================Loading Matrices====================" << endl;
        cout << "Loading matrices with probability: " << percent << endl;
        loadMatrices(Xvalues, Xindices, percent, "X");
        loadMatrices(Yvalues, Yindices, percent, "Y");
        cout << "Matrices loaded!" << endl;

        // Experiement with different threads
        if (mode == "start") {
            cout << "==================Starting Experiments====================" << endl;
            omp_set_num_threads(num_threads);
            cout << "<<<<<<<<<< Evaluating schedulings with probability: " << percent << " and " << num_threads << " threads >>>>>>>>>>" << endl;

            vector<int> chunk_sizes = {0, 100, 200, 500}; // 0 is default chunk size
            vector<string> schedulings = {"static", "dynamic", "guided", "runtime"};

            for (string scheduling : schedulings) {
                for (int chunk_size : chunk_sizes) {
                    cout << "Testing with scheduling: " << scheduling << " and chunk size " << chunk_size << " >>>>>>>>>>" << endl;
                    double start = omp_get_wtime();
                    vector<vector<int>> result = compressedMatrixMultiply(Xvalues, Xindices, Yvalues, Yindices, scheduling, chunk_size);
                    double end = omp_get_wtime();
                    cout << "Elapsed time for " << scheduling << " scheduling and chunk size " << chunk_size << ": " << (end - start) << " seconds" << endl;
                }  
            }
        }
        return 0;
    }
