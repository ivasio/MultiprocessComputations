# Make anyway
.PHONY: lib_obj lib

# User variables
USER_PROGRAM=run.c
COMPILER=gcc
PRE_FLAGS= -g -std=c99 -fopenmp
POST_FLAGS= -L . -lompsort -lm -o run

all: lib ${USER_PROGRAM}
	$(COMPILER) $(PRE_FLAGS) $(USER_PROGRAM) $(POST_FLAGS)

run: all 
	./run 10000000 2000000 4

lib: lib_obj 
	ar -rc libompsort.a ompMergeSort.o
	ranlib libompsort.a

lib_obj: ompMergeSort.c
	gcc -g -std=c99 -fopenmp -c ompMergeSort.c


bench:
	