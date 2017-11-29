#!/bin/bash

max_threads=16
max_repeats=10

repeats=1

while [ $repeats -le $max_repeats ]
do
	./run 10000000 500000 1
	cat stats.txt >> results.csv
	let "repeats += 1"
done


threads=2

while [ $threads -le $max_threads ]
do
	repeats=1

	while [ $repeats -le $max_repeats ]
	do
		./run 10000000 500000 $threads
		cat stats.txt >> results.csv
		let "repeats += 1"
	done

	let "threads *= 2"
done
