#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ompMergeSort.h"


int validateArguments (int argc, char** argv);
int printResults (double time, char** argv, int* array, int* initialArray);

int compare (const void* num1, const void* num2);


int main (int argc, char** argv) {

	if (!validateArguments (argc, argv)) return 1;
	
	int arraySize = atoi (argv[1]);
	int maxChunkSize = atoi (argv[2]);
	int numThreads = atoi (argv[3]);

	
	int* array = (int*)calloc (arraySize, sizeof (int));
	int* initialArray = (int*)calloc (arraySize, sizeof (int));
	
	srand (time (NULL));
	for (int i = 0; i < arraySize; i++) {
		array[i] = rand ();
		initialArray[i] = array[i];
	}
	
	// Empirically found best chunk size = 1/16 of array size
	int chunkSize = (maxChunkSize > arraySize / 16) ? arraySize / 16 : maxChunkSize; 
		
	struct timeval start, end;
	
	gettimeofday(&start, NULL);
	parallelMergeSort (array, arraySize, chunkSize, *compare, numThreads);
	gettimeofday(&end, NULL);
	double deltaMerge = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;


	printResults (deltaMerge, argv, array, initialArray);
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


int validateArguments (int argc, char** argv) {
	
	if (argc != 4) {
		printf ("You must pass 3 arguments, passed %d!\n", argc - 1);
		return 0;
	}
	
	int arraySize = atoi (argv[1]);
	int maxChunkSize = atoi (argv[2]);
	int numThreads = atoi (argv[3]);
	
	
	if (arraySize <= 0) {
		printf ("Size of array to sort must be positive!\n");
		return 0;
	}
	
	if (maxChunkSize <= 0) {
		printf ("Maximal chunk size must be positive!\n");
		return 0;
	}
	
	if (numThreads <= 0) {
		printf ("Number of threads must be positive!\n");
		return 0;
	}
	
	
	return 1;

}
	
int printResults (double time, char** argv, int* array, int* initialArray) {
	
	FILE* outputFile = fopen ("stats.txt", "w");
	
	if (!outputFile) {
		printf ("Could not create stats.txt file! Writing results to the console\n");
		
		printf ("%.4f ", time);
		for (int i = 1; i <= 6; i++) printf ("%s ", argv[i]);
		printf ("\n");		
		
		return 0;
	} 
	
	fprintf (outputFile, "%.4f ", time);
	for (int i = 1; i <= 3; i++) fprintf (outputFile, "%s ", argv[i]);
	fprintf (outputFile, "\n");

	fclose (outputFile);

	size_t arraySize = atoi (argv[1]);
	outputFile = fopen ("data.txt", "w");
	
	if (outputFile) {

		for (int i = 0; i < arraySize; i++) fprintf (outputFile, "%d ", initialArray[i]);
		fprintf (outputFile, "\n");
		for (int i = 0; i < arraySize; i++) fprintf (outputFile, "%d ", array[i]);
		
	}

	fclose (outputFile);	
	return 1;
	
}
