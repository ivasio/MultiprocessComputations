#!/bin/bash

max_threads=16
max_repeats=10
max_N=100000


gcc -std=c99 -D SINGLE_THREAD -fopenmp -o run run.c

N=10

while [ $N -le $max_N ]
do
	repeats=1

	while [ $repeats -le $max_repeats ]
	do
		./run 0 100 50 $N 0.5 1
		cat stats.txt >> results.csv
		let "repeats += 1"
	done

	let "N *= 10"	
done


gcc -std=c99 -fopenmp -o run run.c

N=10

while [ $N -le $max_N ]
do
	threads=2

	while [ $threads -le $max_threads ]
	do
		repeats=1

		while [ $repeats -le $max_repeats ]
		do
			./run 0 100 50 $N 0.5 $threads
			cat stats.txt >> results.csv
			let "repeats += 1"
		done

		let "threads *= 2"
	done

	let "N *= 10"
done