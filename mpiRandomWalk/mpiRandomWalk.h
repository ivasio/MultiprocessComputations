#pragma once 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include <time.h>


// ОСНОВНОЕ

typedef char bool;
enum BOOL {FALSE, TRUE};

enum DIRECTIONS {LEFT, TOP, RIGHT, BOTTOM, LEFT_TOP, RIGHT_TOP, RIGHT_BOTTOM, LEFT_BOTTOM};

enum MPI_TAGS {TAG_NUMBER, TAG_VECTOR};


// ВХОДНЫЕ ПАРАМЕТРЫ ПРОГРАММЫ

typedef struct InputParams {

	int argc;
	char** argv;
	
	int l; 
	int a; 
	int b; 
	int n; 
	int N; 

	double p[4];  // Вероятности переходов

} InputParams;


// ТОЧКИ

typedef struct Point {

	int id;
	int x;
	int y;
	int lifetime;

} Point;


typedef struct PointsVector {

	Point* content;
	int length;
	int size;

} PointsVector;


// КЛЕТКА (ЯЧЕЙКА)

typedef struct Cell {
	
	int length;
	int bounds[4];
	int fieldSizeX;
	int fieldSizeY;
	int neighbours[8];

} Cell;


// ПЕРЕМЕННЫЕ ОКРУЖЕНИЯ ПРОЦЕССА

typedef struct Environment {

	int rank;
	MPI_Datatype* MyMPIpoint;
	int maxSteps;
	Cell* cell;

	PointsVector* points;
	PointsVector* bufferStay;
	PointsVector* buffersSend[8];
	PointsVector* buffersReceive[8];

} Environment;



//////////////////////////////// ФУНКЦИИ


Environment* processInit (int* argc, char*** argv, InputParams* input);

PointsVector* pointsVectorInit (int size);

int getNeighbourRank (int thisRank, int offsetX, int offsetY, int cellsInRow, int cellsInColumn);

bool havePointsToMove (PointsVector* points, int maxSteps);

void movePoints (int maxSteps, PointsVector* points, Cell* cell, double extra, double* probabilities);

void returnPointToTheField (Point* point, Cell* cell);

bool inBounds (Point* point, Cell* cell, int addition);

void pointsExchange (Environment* env);

void fillBufferSend (Cell* cell, PointsVector* points, PointsVector* bufferSend, int direction);

void fillBufferStay (Cell* cell, PointsVector* points, PointsVector* bufferStay);

void pointsVectorResize (PointsVector* vector, int newSize);

int getPointTransitionDirection (Cell* cell, Point* point);

void mergeBuffers (PointsVector* points, PointsVector* bufferStay, PointsVector* buffersReceive[]);

InputParams* getInputParams (int argc, char** argv);

void resultsOutput (int rank, PointsVector* points, InputParams* inputParams, double time);

void processFinalize (InputParams* inputParams, Environment* env);
