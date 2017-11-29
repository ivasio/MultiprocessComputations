all: run.c pthreadMergeSort.o threadPool.o
	gcc -g -std=gnu99 run.c pthreadMergeSort.o threadPool.o -o run -lpthread 

test: test.c pthreadMergeSort.o threadPool.o
	gcc -g -std=gnu99 test.c pthreadMergeSort.o threadPool.o -o run -lpthread 

bench: all
	./script.sh

pthreadMergeSort.o: threadPool.o pthreadMergeSort.c pthreadMergeSort.h
	gcc -g -std=gnu99 -c pthreadMergeSort.c -lpthread


threadPool.o: threadPool.c threadPool.h
	gcc -g -std=gnu99 -c threadPool.c -lpthread