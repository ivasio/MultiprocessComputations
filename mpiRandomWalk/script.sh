#!/bin/sh

make

NODES=$(($2 * $3))
sbatch -n $NODES mpiexec ./run $@
