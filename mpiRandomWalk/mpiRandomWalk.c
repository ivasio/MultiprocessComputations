#include "mpiRandomWalk.h"


Environment* processInit (int* argc, char*** argv, InputParams* input) {

	MPI_Init(NULL, NULL);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Cell* cell = (Cell*) calloc (1, sizeof (Cell));
	cell->length = input->l;

	int cellsInRow = input->a;
	cell->posX = rank % cellsInRow;
	cell->posY = rank / cellsInRow;

	cell->bounds[LEFT] = cell->posX * input->l;
	cell->bounds[RIGHT] = (cell->posX + 1) * input->l;
	cell->bounds[BOTTOM] = cell->posY * input->l;
	cell->bounds[TOP] = (cell->posY + 1) * input->l;

	cell->fieldSizeX = input->a * input->l;
	cell->fieldSizeY = input->b * input->l;


	int cellsInColumn = input->b;

	cell->neighbours[TOP] = getNeighbourRank (rank, 0, 1, cellsInRow, cellsInColumn);
	cell->neighbours[LEFT] = getNeighbourRank (rank, -1, 0, cellsInRow, cellsInColumn);
	cell->neighbours[RIGHT] = getNeighbourRank (rank, 1, 0, cellsInRow, cellsInColumn);
	cell->neighbours[BOTTOM] = getNeighbourRank (rank, 0, -1, cellsInRow, cellsInColumn);
	

	Environment* env = (Environment*) calloc (1, sizeof (Environment));
	env->maxSteps = input->n;
	env->cell = cell;
	env->rank = rank;

	env->points = pointsVectorInit (input->N);
	srand (time (NULL) * rank);
	for (int i = 0; i < input->N; i++) {
		env->points->content[i].id = i + input->N * rank;
		env->points->content[i].x = input->l * cell->posX + rand () % input->l;
		env->points->content[i].y = input->l * cell->posY + rand () % input->l;
		env->points->content[i].lifetime = 0;
	}

	env->bufferStay = pointsVectorInit (input->N);
	for (int direction = 0; direction < 4; direction++) {
		env->buffersSend[direction] = pointsVectorInit (input->N);
		env->buffersReceive[direction] = pointsVectorInit (input->N);
	}

	
	int lengths[1] = {4};
	MPI_Aint offsets[1] = {0};
	MPI_Datatype types[1] = {MPI_INT};
	
	env->MyMPIpoint = (MPI_Datatype*) calloc (1, sizeof (MPI_Datatype));
	MPI_Type_struct (1, lengths, offsets, types, env->MyMPIpoint);
	MPI_Type_commit (env->MyMPIpoint);
	
	return env;

}


PointsVector* pointsVectorInit (int size) {

	PointsVector* vector = (PointsVector*) calloc (2, sizeof (PointsVector));
	vector->content = (Point*)calloc (size, sizeof (Point));
	vector->size = size;
	vector->length = size;

	return vector;

}


int getNeighbourRank (int thisRank, int offsetX, int offsetY, int cellsInRow, int cellsInColumn) {

	int neighX = thisRank % cellsInRow + offsetX;
	if (neighX < 0 || neighX >= cellsInRow) neighX -= cellsInRow * offsetX; 
	int neighY = thisRank / cellsInRow + offsetY;
	if (neighY < 0 || neighY >= cellsInColumn) neighY -= cellsInColumn * offsetY; 
	
	return neighY * cellsInRow + neighX;

}


bool havePointsToMove (PointsVector* points, int maxSteps) {

	int numPointsToMove = 0;
	for (int i = 0; i < points->length; i++) 
		if (points->content[i].lifetime < maxSteps) numPointsToMove++;

	int totalNumPointsToMove = 0;

	MPI_Allreduce ((void*)&numPointsToMove, (void*)&totalNumPointsToMove, 1,
					MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	
	return (totalNumPointsToMove) ? TRUE : FALSE ;

}


void movePoints (int maxSteps, PointsVector* points, Cell* cell, double extra, double* probabilities) {

	int addition = (int) round (extra * cell->length);
	double choice;
	for (int i = 0; i < points->length; i++) {

		returnPointToTheField (&(points->content[i]), cell);

		for (int step = points->content[i].lifetime; step < maxSteps && 
			inBounds (&(points->content[i]), cell, addition); step++) {

				choice = (double)rand () / RAND_MAX;
				if (choice < probabilities[LEFT]) points->content[i].x--;
				else if (choice < probabilities[TOP]) points->content[i].y++;
				else if (choice < probabilities[RIGHT]) points->content[i].x++;
				else points->content[i].y--;  

				points->content[i].lifetime++;
			}

	}

}


bool inBounds (Point* point, Cell* cell, int addition) {

	return 	((  point->x >= cell->bounds[LEFT] &&
				point->x < cell->bounds[RIGHT] &&
				point->y < cell->bounds[TOP] + addition &&
				point->y >= cell->bounds[BOTTOM] - addition
			) || (
				point->x >= cell->bounds[LEFT] - addition &&
				point->x < cell->bounds[RIGHT] + addition &&
				point->y < cell->bounds[TOP] &&
				point->y >= cell->bounds[BOTTOM]
			));

}


void returnPointToTheField (Point* point, Cell* cell) {

	if (point->x < 0) point->x += cell->fieldSizeX;
	else if (point->x >= cell->fieldSizeX) point->x -= cell->fieldSizeX;
	
	if (point->y < 0) point->y += cell->fieldSizeY;
	else if (point->y >= cell->fieldSizeY) point->y -= cell->fieldSizeY;

}


void pointsExchange (Environment* env) {

	if (env->cell->posY == 0) {
		pointsSwap (env, TOP, SEND_FIRST);
		pointsSwap (env, BOTTOM, RECEIVE_FIRST);
		
	} else {
		pointsSwap (env, BOTTOM, RECEIVE_FIRST);
		pointsSwap (env, TOP, SEND_FIRST);
	}

	if (env->cell->posX == 0) {
		pointsSwap (env, RIGHT, SEND_FIRST);
		pointsSwap (env, LEFT, RECEIVE_FIRST);
		
	} else {
		pointsSwap (env, LEFT, RECEIVE_FIRST);
		pointsSwap (env, RIGHT, SEND_FIRST);
	}

	fillBufferStay (env->cell, env->points, env->bufferStay);
	mergeBuffers (env->points, env->bufferStay, env->buffersReceive);

}

void pointsSwap (Environment* env, int direction, int actionOrder) {

		int numPointsToReceive;
		MPI_Status status;

		fillBufferSend (env->cell, env->points, env->buffersSend[direction], direction);
		MPI_Sendrecv (&env->buffersSend[direction]->length, 1, MPI_INT, env->cell->neighbours[direction], TAG_NUMBER,
			&numPointsToReceive, 1, MPI_INT, env->cell->neighbours[direction], TAG_NUMBER, MPI_COMM_WORLD, &status);

		if (actionOrder == SEND_FIRST) {

			if (env->buffersSend[direction]->length)
				MPI_Send ((void*)env->buffersSend[direction]->content, 
					env->buffersSend[direction]->length, *(env->MyMPIpoint), 
					env->cell->neighbours[direction], TAG_VECTOR, MPI_COMM_WORLD);	

		}

		env->buffersReceive[direction]->length = numPointsToReceive;
		if (numPointsToReceive) {
			
			if (numPointsToReceive > env->buffersReceive[direction]->size)
				pointsVectorResize (env->buffersReceive[direction], numPointsToReceive);

			MPI_Recv ((void*)env->buffersReceive[direction]->content, numPointsToReceive, *(env->MyMPIpoint), 
				env->cell->neighbours[direction], TAG_VECTOR, MPI_COMM_WORLD, &status);
		}

		if (actionOrder == RECEIVE_FIRST) {

			if (env->buffersSend[direction]->length)
				MPI_Send ((void*)env->buffersSend[direction]->content, 
					env->buffersSend[direction]->length, *(env->MyMPIpoint), 
					env->cell->neighbours[direction], TAG_VECTOR, MPI_COMM_WORLD);	

		} 

}


void fillBufferSend (Cell* cell, PointsVector* points, PointsVector* bufferSend, int direction) { 

	int pointsToSend = 0;
	for (int i = 0; i < points->length; i++)
		if (!inBounds (&(points->content[i]), cell, 0) && 
			getPointTransitionDirection (cell, &(points->content[i])) == direction) pointsToSend++;

	if (pointsToSend) {

		if (pointsToSend > bufferSend->size) 
			pointsVectorResize (bufferSend, pointsToSend);

		int index = 0;
		for (int i = 0; i < points->length; i++) 
			if (!inBounds (&(points->content[i]), cell, 0) && 
				getPointTransitionDirection (cell, &(points->content[i])) == direction) {
					bufferSend->content[index] = points->content[i];
					index++;
				}

	}	
	
	bufferSend->length = pointsToSend;

}


void fillBufferStay (Cell* cell, PointsVector* points, PointsVector* bufferStay) { 

	int index = 0;
	for (int i = 0; i < points->length; i++) 
		if (inBounds (&(points->content[i]), cell, 0)) {
			bufferStay->content[index] = points->content[i];
			index++;
		}

	bufferStay->length = index;

}


void pointsVectorResize (PointsVector* vector, int newSize) {

	vector->content = (Point*)realloc (vector->content, newSize * sizeof (Point));
	vector->size = newSize;

}


int getPointTransitionDirection (Cell* cell, Point* point) {

	if (point->y >= cell->bounds[TOP]) 
		return TOP;
	else if (point->y < cell->bounds[BOTTOM]) 
		return BOTTOM;
	else if (point->x < cell->bounds[LEFT]) 
		return LEFT;
	else 
		return RIGHT;

}


void mergeBuffers (PointsVector* points, PointsVector* bufferStay, PointsVector* buffersReceive[]) {

	int rank;
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	int totalPointsNumber = bufferStay->length;
	for (int direction = 0; direction < 4; direction++)
		totalPointsNumber += buffersReceive[direction]->length;

	if (points->size < totalPointsNumber) {
		pointsVectorResize (points, totalPointsNumber);
		pointsVectorResize (bufferStay, totalPointsNumber);
	}

	points->length = totalPointsNumber;
	
	int offset = 0;
	if (bufferStay->length) {
		memcpy ((void*)(points->content), (void*)(bufferStay->content), 
					bufferStay->length * sizeof (Point));
		offset += bufferStay->length;
	}

	for (int direction = 0; direction < 4; direction++) {
		if (buffersReceive[direction]->length) {
			memcpy ((void*)(points->content + offset), (void*)(buffersReceive[direction]->content), 
					buffersReceive[direction]->length * sizeof (Point));
			offset += buffersReceive[direction]->length;
			buffersReceive[direction]->length = 0;
		}
	}


}


void resultsOutput (int rank, PointsVector* points, InputParams* inputParams, double time) {

	if (rank > 0) {
		
		MPI_Gather (&(points->length), 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
		
	} else {
		
		int cellsTotally = inputParams->a * inputParams->b;
		int* results = (int*) calloc (cellsTotally, sizeof (int));

		MPI_Gather (&(points->length), 1, MPI_INT, results, 1, MPI_INT, 0, MPI_COMM_WORLD);
		
		FILE* outputFile = fopen ("stats.txt", "w");
		if (!outputFile) {
			printf ("Could not create results.csv file!\n");
			return;
		}

		for (int i = 1; i < inputParams->argc; i++) fprintf (outputFile, "%s ", inputParams->argv[i]); 
		fprintf (outputFile, "%.2lg\n", time);

		for (int i = 0; i < cellsTotally; i++)
			fprintf (outputFile, "%d: %d\n", i, results[i]);

		fclose (outputFile);
		free (results);

	}

}


void processFinalize (InputParams* inputParams, Environment* env) {

	void* buf;
	int bufSize;
	MPI_Buffer_detach (buf, &bufSize);
	free (env->mpiBuffer);

	free (inputParams);

	free (env->MyMPIpoint);
	free (env->cell);
	free (env->points->content);
	free (env->points);
	free (env->bufferStay->content);
	free (env->bufferStay);
	
	for (int i = 0; i < 4; i++) {

		free (env->buffersSend[i]->content);
		free (env->buffersSend[i]);
		free (env->buffersReceive[i]->content);
		free (env->buffersReceive[i]);

	}

	free (env);

	MPI_Finalize ();

}
