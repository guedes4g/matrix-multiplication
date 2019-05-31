mpicc main.c -o paralelo -DSIZE=15 

mpirun -np 2 ./paralelo 