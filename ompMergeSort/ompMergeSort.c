#include <stdlib.h>
#include <omp.h>
#include <math.h>

#include "ompMergeSort.h"

void parallelMergeSort (int* array, size_t arraySize, size_t chunkSize,
						int (*compare) (const void*, const void*), int numberOfThreads) {	

//	omp_set_dynamic (0);
	omp_set_num_threads (numberOfThreads);
	int* initialArray = array;
	
	#pragma omp parallel 
	{
		
		#pragma omp master 
		{
			for (int i = 0; i < arraySize / chunkSize; i++) {
				#pragma omp task 
				{
					qsort (array + i * chunkSize, chunkSize,
							sizeof (int), (__compar_fn_t)compare);
				}
			}
			
			#pragma omp taskwait

			if (arraySize % chunkSize != 0) {
				int elementsLeft = arraySize % chunkSize;
				qsort (array + arraySize - elementsLeft, elementsLeft,
						sizeof (int), (__compar_fn_t)compare);
			}
			
			int* array1 = array;
			int* array2 = (int*)calloc (arraySize, sizeof (int));
			
			bounds_args_t bounds = {array1, arraySize, chunkSize, 0, 0, 0, 0};
			int depth = getBounds (&bounds); // initialization and getting the tree depth
			
			for (int i = 0; i <= depth; i++) {
		
				int boundsResult;
				while ((boundsResult = getBounds (&bounds)) > 0) {
				
					#pragma omp task 
					{
						merge (array1 + bounds.left1, array1 + bounds.left2,
								array1 + bounds.median1 - 1, array1 + bounds.median2 - 1,
								array2 + bounds.left1, compare);
						
					}
					
					#pragma omp task 
					{
						merge (array1 + bounds.median1, array1 + bounds.median2,
								array1 + bounds.right1, array1 + bounds.right2,
								array2 + bounds.left1 + (bounds.median1 - bounds.left1) +
								(bounds.median2 - bounds.left2), compare);
					}
				
				}

				#pragma omp taskwait
										
				if (boundsResult == 0 && bounds.left1 <= bounds.right1) {
					for (size_t ind = bounds.left1; ind <= bounds.right1; ind++)
					array2[ind] = array1[ind];
				}
				
				if (boundsResult == -1) break;


				int* temp = array1;
				array1 = array2;
				array2 = temp;
				
				bounds.array = array1;
				
			}
			
			if (array1 != initialArray) {
				for (int i = 0; i < arraySize; i++)
					array2[i] = array1[i];
				free (array1);
			} else 
				free (array2);
		}
	}
	
}



int getBounds (bounds_args_t* args) {

	static char initialized = 0;
	static size_t arraySize, depth, chunkSize, currentLevel;
	static int left1, left2, right1, right2;
	int* array = args->array;
	
	
	if (!initialized) {

		array = args->array;
		arraySize = args->arraySize;
		chunkSize = args->chunkSize; // this value can be improved
		
		double chunksFraction = (double)arraySize / chunkSize;
		depth = (size_t) ceil (log (chunksFraction) / log (2));
		currentLevel = 0;
		
		left1 = -2 * chunkSize;
		right1 = -chunkSize - 1;
		left2 = -chunkSize;
		right2 = -1;
		
		initialized++;
		return depth;
		
	} else { // has been already initialized


		if (currentLevel < depth) { // the tree has not been traversed completely yet

			size_t elementsLeft = arraySize - right2 - 1;
			size_t median1, median2;

			int returnValue;

			if (elementsLeft / chunkSize >= 2) { // enough complete chunks to merge

				returnValue = 1;
				
				left1 += 2 * chunkSize;
				left2 += 2 * chunkSize;
				right1 += 2 * chunkSize;
				right2 += 2 * chunkSize;

				median1 = left1 + chunkSize / 2;
				median2 = left2 + chunkSize / 2;
				
			} else if (elementsLeft / chunkSize == 1 &&
					   elementsLeft % chunkSize >= 1) { // one complete and one incomplete chunk

				returnValue = 2;
				
				left1 += 2 * chunkSize;
				left2 += 2 * chunkSize;
				right1 += 2 * chunkSize;
				right2 += chunkSize + (elementsLeft % chunkSize);

				median1 = left1 + chunkSize / 2;
				median2 = left2 + (elementsLeft - chunkSize) / 2;

			} else { // one complete or incomplete chunk OR no chunks

				currentLevel++;
				chunkSize *= 2;

				// setting values for the next level
				left1 = -2 * chunkSize;
				right1 = -chunkSize - 1;
				left2 = -chunkSize;
				right2 = -1;

				// returning bounds of the last piece to copy
				args->left1 = arraySize - elementsLeft;
				args->right1 = arraySize - 1;

				return 0; // nothing to merge on this level
				
			}

			if (array[median2] < array[median1]) {
				
				//if (median2 > right2) no 2 part, just replacing the 1 part while merging
				while (array[median2] < array[median1] && median2 <= right2)
					median2++;

			} else {

				//if (median2 == left2) no 1 part, just replacing the 2 part while merging
				while (array[median2] >= array[median1] && median2 >= left2) median2--; 
				median2++;
					
			}

			args->left1 = left1;
			args->left2 = left2;
			args->right1 = right1;
			args->right2 = right2;
			args->median1 = median1;
			args->median2 = median2;

							   
			return returnValue;
			
		} else { // the tree is traversed completely

			initialized--;
			return -1;  

		}
	}
	
}


void merge (int* left1, int* left2, int* right1, int* right2, 
			int* newArray, int (*compare) (const void*, const void*)) {

	while (left1 <= right1 && left2 <= right2) {

		if (compare (left1, left2) < 0) {
			*newArray = *left1;
			left1++;
		} else {
			*newArray = *left2;
			left2++;
		}

		newArray++;
	}

	if (left1 > right1) {

		while (left2 <= right2) {
			*newArray = *left2;
			left2++;
			newArray++;
		}

	} else {

		while (left1 <= right1) {
			*newArray = *left1;
			left1++;
			newArray++;
		}

	}

}


void swap (int* elem1, int* elem2) {
	int temp = *elem1;
	*elem1 = *elem2;
	*elem2 = temp;
}

