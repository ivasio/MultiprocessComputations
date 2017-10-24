#!/bin/bash

max_threads=16
max_repeats=10



gcc -std=c99 -fopenmp -D SINGLE_THREAD run.c -L . -lompsort -lm -o run
repeats=1

while [ $repeats -le $max_repeats ]
do
	./run 10000000 1000000 1
	cat stats.txt >> results.csv
	let "repeats += 1"
done


gcc -std=c99 -fopenmp run.c -L . -lompsort -lm -o run

threads=2

while [ $threads -le $max_threads ]
do
	repeats=1

	while [ $repeats -le $max_repeats ]
	do
		./run 10000000 1000000 $threads
		cat stats.txt >> results.csv
		let "repeats += 1"
	done

	let "threads *= 2"
done
