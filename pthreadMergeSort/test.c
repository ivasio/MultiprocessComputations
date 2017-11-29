#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

typedef int type;
#define DEBUG
#include "../customDebugging.h"


typedef int elemType;
#include "pthreadMergeSort.h"

int compare (const void* nuamber1, const void* number2);


int main (int argc, char** argv) {

	int arraySize = atoi (argv[1]);
	int chunkSize = atoi (argv[2]);
	int threadsNum = atoi (argv[3]);
	int* array = (int*)calloc (arraySize, sizeof (int));
	int* initialArray = (int*)calloc (arraySize, sizeof (int));
	
	srand (time (NULL));
	for (int i = 0; i < arraySize; i++) {
		array[i] = rand ();
		initialArray[i] = array[i];
	}

	struct timeval start1, end1, start2, end2;
	
	gettimeofday(&start1, NULL);
	parallelMergeSort (array, arraySize, chunkSize, compare, threadsNum);
	gettimeofday(&end1, NULL);
	double delta1 = ((end1.tv_sec - start1.tv_sec) * 1000000u + end1.tv_usec - start1.tv_usec) / 1.e6;
	
	gettimeofday(&start2, NULL);
	qsort (initialArray, arraySize, sizeof (int), (__compar_fn_t)compare);
	gettimeofday(&end2, NULL);
	double delta2 = ((end2.tv_sec - start2.tv_sec) * 1000000u + end2.tv_usec - start2.tv_usec) / 1.e6;

	(delta1 < delta2) ? printf ("Yeeeaa\n") : printf ("Nooooo\n");
	int result = 1;
	for (int i = 0; i < arraySize; i++) 
		if (array[i] != initialArray[i]) {
			result = 0;
			printf ("%d : %d vs %d\n", i, array[i], initialArray[i]);
		}

	(result == 1) ? printf ("Success!\n") : printf ("Fail!\n");
	
	free (array);
	free (initialArray);
	
	
	return 0;

}

int compare (const void* number1, const void* number2) {
	
	int num1 = *(int*)number1, num2 = *(int*)number2;
	
	if (num1 < num2) 
		return -1;
	else if (num1 == num2)
		return 0;
	else 
		return 1;
		
}

