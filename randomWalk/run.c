#define _XOPEN_SOURCE		// for rand_r ()

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <omp.h>
#include <sys/time.h>


 
typedef struct args_t {
	
	int startPosition;
	int leftBorder;
	int rightBorder;
	double probability;
	int particles;
	
	double resultProbability;
	double resultLifetime;
	
} args_t;


int validateArguments (int argv, char** argc);
void walk (args_t* args);
int printResults (double probability, double lifetime, double runningTime, char **argc);

int main (int argv, char** argc) {
	
	if (!validateArguments (argv, argc)) return 1;
	
	int leftBorder = atoi (argc[1]);
	int rightBorder = atoi (argc[2]);
	int startPosition = atoi (argc[3]);
	int particles = atoi (argc[4]);
	double probability = strtod (argc[5], NULL);
	int threads = atoi (argc[6]);
	
	omp_set_num_threads (threads);
	
	
	args_t arguments = {startPosition, leftBorder, rightBorder, probability, particles, 0, 0};
	
	struct timeval start, end;
	
	gettimeofday(&start, NULL);
	
	walk (&arguments);

	gettimeofday(&end, NULL);

	double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
	
	printResults (arguments.resultProbability, arguments.resultLifetime, delta, argc);
	
	return 0;
	
}

void walk (args_t* args) {
	
	int position, sum = 0, totalSteps = 0;
	srand(time(NULL));
	unsigned int randState = rand();
	double choice;
	
	#ifndef SINGLE_THREAD
	#pragma omp parallel for private(position,choice,randState) reduction(+: sum,totalSteps)
	#endif
	for (int i = 0; i < args->particles; i++) {
		
		position = args->startPosition;
		randState = rand ();
		
		while (position != args->leftBorder && position != args->rightBorder) {
			totalSteps++;
			choice = (double)rand_r(&randState) / RAND_MAX;
			(choice > args->probability) ? position-- : position++;
		}
		
		if (position == args->rightBorder)
			sum++;
			
	}
	
	args->resultProbability = (double)sum / args->particles;
	args->resultLifetime = (double)totalSteps / args->particles;
	
}

int validateArguments (int argv, char** argc) {
	
	if (argv != 7) {
		printf ("You must pass 6 arguments, passed %d!\n", argv - 1);
		return 0;
	}
	
	int leftBorder = atoi (argc[1]);
	int rightBorder = atoi (argc[2]);
	int startPosition = atoi (argc[3]);
	int particles = atoi (argc[4]);
	double probability = strtod (argc[5], NULL);
	int threads = atoi (argc[6]);
	
	
	if (rightBorder <= leftBorder) {
		printf ("Right border must be greater than left border!\n");
		return 0;
	}
	
	if (startPosition <= leftBorder) {
		printf ("Start position must be greater than left border!\n");
		return 0;
	}
	
	if (startPosition >= rightBorder) {
		printf ("Start position must be less than right border!\n");
		return 0;
	}
	
	if (probability >= 1 || probability <= 0){
		printf ("Probabitity value must be in range of [0;1] (excluding)!\n");
		return 0;
	}
	
	if (particles <= 0) {
		printf ("Number of particles must be positive!\n");
		return 0;
	}
	
	if (threads <= 0) {
		printf ("Number of threads must be positive!\n");
		return 0;
	}
	
	
	return 1;

}
	
int printResults (double probability, double lifetime, double runningTime, char **argc) {
	
	FILE* outputFile = fopen ("stats.txt", "w");
	
	if (!outputFile) {
		printf ("Could not create results.csv file! Writing results to the console\n");
		
		printf ("%.2f %.2f %.4fs ", probability, lifetime, runningTime);
		for (int i = 1; i <= 6; i++) printf ("%s ", argc[i]);
		printf ("\n");		
		
		return 0;
	} 
	
	fprintf (outputFile, "%.2f %.2f %.4fs ", probability, lifetime, runningTime);
	for (int i = 1; i <= 6; i++) fprintf (outputFile, "%s ", argc[i]);
	fprintf (outputFile, "\n");
	
	return 1;
	
}
