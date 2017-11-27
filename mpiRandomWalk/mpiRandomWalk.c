#include "mpiRandomWalk.h"


Environment* processInit (int* argc, char*** argv, InputParams* input) {

	MPI_Init(NULL, NULL);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Cell* cell = (Cell*) calloc (1, sizeof (Cell));
	cell->length = input->l;

	int cellsInRow = input->a;
	int cellPosX = rank % cellsInRow;
	int cellPosY = rank / cellsInRow;

	cell->bounds[LEFT] = cellPosX * input->l;
	cell->bounds[RIGHT] = (cellPosX + 1) * input->l;
	cell->bounds[BOTTOM] = cellPosY * input->l;
	cell->bounds[TOP] = (cellPosY + 1) * input->l;

	cell->fieldSizeX = input->a * input->l;
	cell->fieldSizeY = input->b * input->l;


	int cellsInColumn = input->b;

	cell->neighbours[LEFT_TOP] = getNeighbourRank (rank, -1, 1, cellsInRow, cellsInColumn);
	cell->neighbours[TOP] = getNeighbourRank (rank, 0, 1, cellsInRow, cellsInColumn);
	cell->neighbours[RIGHT_TOP] = getNeighbourRank (rank, 1, 1, cellsInRow, cellsInColumn);
	cell->neighbours[LEFT] = getNeighbourRank (rank, -1, 0, cellsInRow, cellsInColumn);
	cell->neighbours[RIGHT] = getNeighbourRank (rank, 1, 0, cellsInRow, cellsInColumn);
	cell->neighbours[LEFT_BOTTOM] = getNeighbourRank (rank, -1, -1, cellsInRow, cellsInColumn);
	cell->neighbours[BOTTOM] = getNeighbourRank (rank, 0, -1, cellsInRow, cellsInColumn);
	cell->neighbours[RIGHT_BOTTOM] = getNeighbourRank (rank, 1, -1, cellsInRow, cellsInColumn);


	Environment* env = (Environment*) calloc (1, sizeof (Environment));
	env->maxSteps = input->n;
	env->cell = cell;
	env->rank = rank;

	env->points = pointsVectorInit (input->N);
	srand (time (NULL) * rank);
	for (int i = 0; i < input->N; i++) {
		env->points->content[i].id = i + input->N * rank;
		env->points->content[i].x = input->l * cellPosX + rand () % input->l;
		env->points->content[i].y = input->l * cellPosY + rand () % input->l;
		env->points->content[i].lifetime = 0;
	}

	env->bufferStay = pointsVectorInit (input->N);
	for (int direction = 0; direction < 8; direction++) {
		env->buffersSend[direction] = pointsVectorInit (input->N);
		env->buffersReceive[direction] = pointsVectorInit (input->N);
	}

	
	int lengths[1] = {4};
	MPI_Aint offsets[1] = {0};
	MPI_Datatype types[1] = {MPI_INT};
	
	env->MyMPIpoint = (MPI_Datatype*) calloc (1, sizeof (MPI_Datatype));
	MPI_Type_struct (1, lengths, offsets, types, env->MyMPIpoint);
	MPI_Type_commit (env->MyMPIpoint);

	int mpiBufSize = sizeof (Point) * input->N * 2 + 8 * MPI_BSEND_OVERHEAD;
	env->mpiBuffer = malloc (mpiBufSize);
	MPI_Buffer_attach (env->mpiBuffer, mpiBufSize);
	
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

	return  point->x >= cell->bounds[LEFT] - addition &&
			point->x < cell->bounds[RIGHT] + addition &&
			point->y < cell->bounds[TOP] + addition &&
			point->y >= cell->bounds[BOTTOM] - addition;

}


void returnPointToTheField (Point* point, Cell* cell) {

	if (point->x < 0) point->x += cell->fieldSizeX;
	else if (point->x >= cell->fieldSizeX) point->x -= cell->fieldSizeX;
	
	if (point->y < 0) point->y += cell->fieldSizeY;
	else if (point->y >= cell->fieldSizeY) point->y -= cell->fieldSizeY;

}


void pointsExchange (Environment* env) {

	for (int direction = 0; direction < 8; direction++) {

		fillBufferSend (env->cell, env->points, env->buffersSend[direction], direction);
		MPI_Bsend ((void*)&(env->buffersSend[direction]->length), 1, MPI_INT, 
			env->cell->neighbours[direction], TAG_NUMBER, MPI_COMM_WORLD);

		if (env->buffersSend[direction]->length)
			MPI_Bsend ((void*)env->buffersSend[direction]->content, 
				env->buffersSend[direction]->length, *(env->MyMPIpoint), 
				env->cell->neighbours[direction], TAG_VECTOR, MPI_COMM_WORLD);

	}

	MPI_Status status;
	int numPointsToReceive;
	for (int direction = 0; direction < 8; direction++) {

		int receiveDirection = (direction % 4 + 2) % 4 + (direction / 4) * 4;

		MPI_Recv ((void*)&numPointsToReceive, 1, MPI_INT, 
			env->cell->neighbours[receiveDirection], TAG_NUMBER, MPI_COMM_WORLD, &status);

		env->buffersReceive[receiveDirection]->length = numPointsToReceive;

		if (numPointsToReceive) {

			if (numPointsToReceive > env->buffersReceive[receiveDirection]->size)
				pointsVectorResize (env->buffersReceive[receiveDirection], numPointsToReceive);

			MPI_Recv ((void*)env->buffersReceive[receiveDirection]->content, numPointsToReceive, *(env->MyMPIpoint), 
				env->cell->neighbours[receiveDirection], TAG_VECTOR, MPI_COMM_WORLD, &status);

		}

	}

	fillBufferStay (env->cell, env->points, env->bufferStay);
	mergeBuffers (env->points, env->bufferStay, env->buffersReceive);

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

	if (point->y >= cell->bounds[TOP]) {

		if (point->x < cell->bounds[LEFT])
			return LEFT_TOP;
		else if (point->x < cell->bounds[RIGHT])
			return TOP;
		else
			return RIGHT_TOP;

	} else if (point->y < cell->bounds[BOTTOM]) {

		if (point->x < cell->bounds[LEFT])
			return LEFT_BOTTOM;
		else if (point->x < cell->bounds[RIGHT])
			return BOTTOM;
		else
			return RIGHT_BOTTOM;

	} else {

		if (point->x < cell->bounds[LEFT])
			return LEFT;
		else
			return RIGHT;

	}

}


void mergeBuffers (PointsVector* points, PointsVector* bufferStay, PointsVector* buffersReceive[]) {

	int rank;
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	int totalPointsNumber = bufferStay->length;
	for (int direction = 0; direction < 8; direction++)
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

	for (int direction = 0; direction < 8; direction++) {
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
	
	for (int i = 0; i < 8; i++) {

		free (env->buffersSend[i]->content);
		free (env->buffersSend[i]);
		free (env->buffersReceive[i]->content);
		free (env->buffersReceive[i]);

	}

	free (env);

	MPI_Finalize ();

}
