mpicc main.c -o paralelo -fopenmp -DSIZE=1500

mpirun -np 4 ./paralelo