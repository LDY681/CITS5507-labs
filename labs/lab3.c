#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
  // Array of floats
  int array_size = 276447232;
  float *array = (float*)malloc(array_size * sizeof(float));
  float arraySum = 0;

  // initialize array and find the sum;
  for (int i = 0; i < array_size; i++) {
      float item = ((float)rand() / RAND_MAX) * 1.0f;
      array[i] = item; // Numbers between 0 and 1
      arraySum += item;
  }

  // start clock on execution
  clock_t start_time = clock();

  float totalSum;
  float localSum;
  #pragma omp parallel private(localSum) reduction(+:totalSum)
  {
    #pragma omp for
      for (int i = 0; i < array_size; i++) {
          localSum += array[i];
      }
    //Code here will be executed by all threads
    printf("localSum %f in thread %d\n", localSum, omp_get_thread_num());
    totalSum += localSum;
    // Make sure all threads see the most up-to-date totalSum
    #pragma omp flush(totalSum)
  }
  printf("totalSum in all threads %f\n Actual Sum of the array %f\n", totalSum, arraySum);



  // Record the end time
  clock_t end_time = clock();

  // Calculate the elapsed time in seconds
  double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
  // Print the elapsed time
  printf("Time taken to complete sum task: %f seconds\n", time_spent);
  return 0;
}
