#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// DADOS COMPARTILHADOS
int m1[SIZE][SIZE], m2[SIZE][SIZE], mres[SIZE][SIZE];
int l1, c1, l2, c2, lres, cres;

int inicializa_matriz(int i, int j, int k);
int multiplica_matriz(int i, int j, int k, int offset, int step_size);
int valida_multiplicacao(int i, int j, int k);

int main(int argc, char *argv[]) {
  int i, j, k, id, p, nWorkers, offset, stop, step_size, rows, cols,
      waitingList, offsetR, masterId;
  double elapsed_time;
  MPI_Status status;  // estrutura que guarda o estado de retorno
  masterId = 0;
  offsetR = 0;
  offset = 0;
  stop = 0;
  rows = SIZE;
  // inicializa o MPI, recebe o endereço dos parâmetros da função main()
  MPI_Init(&argc, &argv);
  // pega informacao do numero de processos em execucao (quantidade total)s
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  // pega o identificador do processo atual (rank)
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  nWorkers = p - 1;
  // Escravo
  if (id != masterId) {
    MPI_Recv(&m2, SIZE * SIZE, MPI_INT, masterId, 1, MPI_COMM_WORLD, &status);
    while (1) {
      MPI_Recv(&stop, 1, MPI_INT, masterId, 2, MPI_COMM_WORLD, &status);
      if (stop) break;
      MPI_Recv(&offset, 1, MPI_INT, masterId, 3, MPI_COMM_WORLD, &status);
      MPI_Recv(&step_size, 1, MPI_INT, masterId, 4, MPI_COMM_WORLD, &status);
      MPI_Recv(&m1[offset][0], step_size * SIZE, MPI_DOUBLE, masterId, 5,
               MPI_COMM_WORLD, &status);

      multiplica_matriz(i, j, k, offset, step_size);
      // printMres();
      MPI_Send(&offset, 1, MPI_INT, masterId, 6, MPI_COMM_WORLD);
      MPI_Send(&step_size, 1, MPI_INT, masterId, 7, MPI_COMM_WORLD);
      MPI_Send(&mres[offset][0], step_size * SIZE, MPI_INT, masterId, 8,
               MPI_COMM_WORLD);
    }
    MPI_Finalize();
    exit(0);
  }
  // Mestre
  else {
    inicializa_matriz(i, j, k);
    // PREPARA PARA MEDIR TEMPO
    elapsed_time = -MPI_Wtime();
    for (i = 1; i <= nWorkers; i++) {
      MPI_Send(&m2, SIZE * SIZE, MPI_INT, i, 1, MPI_COMM_WORLD);
    }
    while (rows > 0) {
      waitingList = 0;
      for (i = 1; i <= nWorkers; i++) {
        step_size = i == 1 ? 15 : 16;
        step_size = step_size > rows ? rows : step_size;

        MPI_Send(&stop, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
        MPI_Send(&offset, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
        MPI_Send(&step_size, 1, MPI_INT, i, 4, MPI_COMM_WORLD);
        MPI_Send(&m1[offset][0], step_size * SIZE, MPI_INT, i, 5,
                 MPI_COMM_WORLD);
        offset = offset + step_size;
        rows = rows - step_size;
        waitingList++;
        if (rows <= 0) break;
      }
      for (i = 1; i <= waitingList; i++) {
        MPI_Recv(&offsetR, 1, MPI_INT, i, 6, MPI_COMM_WORLD, &status);
        MPI_Recv(&step_size, 1, MPI_INT, i, 7, MPI_COMM_WORLD, &status);
        MPI_Recv(&mres[offsetR][0], step_size * SIZE, MPI_DOUBLE, i, 8,
                 MPI_COMM_WORLD, &status);
      }
    }
    stop = 1;
    for (i = 1; i <= nWorkers; i++) {
      MPI_Send(&stop, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
    }
    // OBTEM O TEMPO
    elapsed_time += MPI_Wtime();
    // VERIFICA SE O RESULTADO DA MULTIPLICACAO ESTA CORRETO
    int correct_mult = valida_multiplicacao(i, j, k);

    // MOSTRA O TEMPO DE EXECUCAO
    printf("Correct;processes;time;\n");
    printf("%s;%d;%lf;\n", !correct_mult ? "True" : "False", p, elapsed_time);
    MPI_Finalize();
    return 0;
  }
}

int multiplica_matriz(int i, int j, int k, int offset, int step_size) {
#pragma omp parallel for num_threads(step_size) private(i, j, k)
  for (i = offset; i < step_size + offset; i++) {
    for (j = 0; j < SIZE; j++) {
      mres[i][j] = 0;
      for (k = 0; k < SIZE; k++) {
        mres[i][j] += m1[i][k] * m2[k][j];
      }
    }
  }
}

int inicializa_matriz(int i, int j, int k) {
  // INICIALIZA OS ARRAYS A SEREM MULTIPLICADOS
  l1 = c1 = SIZE;
  l2 = c2 = SIZE;
  if (c1 != l2) {
    fprintf(stderr, "Impossivel multiplicar matrizes: parametros invalidos.\n");
    return 1;
  }
  lres = l1;
  cres = c2;
  k = 1;
  for (i = 0; i < SIZE; i++) {
    for (j = 0; j < SIZE; j++) {
      if (k % 2 == 0)
        m1[i][j] = -k;
      else
        m1[i][j] = k;
    }
    k++;
  }
  k = 1;
  for (j = 0; j < SIZE; j++) {
    for (i = 0; i < SIZE; i++) {
      if (k % 2 == 0)
        m2[i][j] = -k;
      else
        m2[i][j] = k;
    }
    k++;
  }
}

int valida_multiplicacao(int i, int j, int k) {
  for (i = 0; i < SIZE; i++) {
    k = SIZE * (i + 1);
    for (j = 0; j < SIZE; j++) {
      int k_col = k * (j + 1);
      if (i % 2 == 0) {
        if (j % 2 == 0) {
          if (mres[i][j] != k_col) return 1;
        } else {
          if (mres[i][j] != -k_col) return 1;
        }
      } else {
        if (j % 2 == 0) {
          if (mres[i][j] != -k_col) return 1;
        } else {
          if (mres[i][j] != k_col) return 1;
        }
      }
    }
  }
  return 0;
}