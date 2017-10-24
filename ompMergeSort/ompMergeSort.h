#pragma once

/*======================================================================
 *	The main sorting function
 *	Takes array pointer @array@, its size @arraySize@,
 * 	already calculated @chunkSize@, comparator function
 * 	pointer @compare@ and number of threads @numberOfThreads@
 *
 *	Works resursively : divides the array into chunks, sorts them
 * 	via quicksort and merges them. OMP tasks are used on every level,
 * 	they get parts' bounds from the getBounds() function. Chunk size
 * 	grows twice when going to the next level
 * ===================================================================*/
 
void parallelMergeSort (int* array, size_t arraySize, size_t chunkSize,
						int (*compare) (const void*, const void*), int numberOfThreads);


/*======================================================================
 *	Structure to pass arguments to and get results from the getBounds().
 *
 *	When called the first time, getBounds() is being initialized and
 *  gets array pointer @array@, size @arraySize@ and initial chunk size
 *  @chunkSize@. All the other time until the array is completely sorted,
 * 	it returns left, right borders (including) and medians of two parts
 *  of an array via this structure.
 * ===================================================================*/
 
typedef struct bounds_args_t {

	int* array;
	size_t arraySize;
	size_t chunkSize;
	size_t left1;
	size_t left2;
	size_t right1;
	size_t right2;
	size_t median1;
	size_t median2;
	
} bounds_args_t;


/*======================================================================
 *	Uses a bunch of static variables to store important values and has
 *  also quite comlicated logic. Usage of the @args@ structure pointer
 *  is described above.
 *  Returns 1 when there're two comlete chunks to merge, 2 when one
 *  complete and one incomplete, 0 when one incomplete and -1 when the
 *  whole array is finally sorted.
 * ===================================================================*/
						
int getBounds (bounds_args_t* args);


/*======================================================================
 *	Merges parts of two close chunks bordered by [@left1@ ; @right1@]
 *  and [@left2@ ; @right2@] and places the resulting array in @newArray@
 * ===================================================================*/

void merge (int* left1, int* left2, int* right1, int* right2, 
			int* newArray, int (*compare) (const void*, const void*));

/*======================================================================
 *	Swaps two elements of the array, pointed by @elem1@ and @elem2@
 * ===================================================================*/
 
void swap (int* elem1, int* elem2);

