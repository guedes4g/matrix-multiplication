mpicc sequencial.c -o sequencial-1500 -DSIZE=1500 

mpirun -np 1 ./sequencial-1500 