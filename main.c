#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// DADOS COMPARTILHADOS
int m1[SIZE][SIZE], m2[SIZE][SIZE], mres[SIZE][SIZE];
int l1, c1, l2, c2, lres, cres;

int inicializa_matriz(int i, int j, int k);
int multiplica_matriz(int i, int j, int k);
int valida_multiplicacao(int i, int j, int k);

// comunicacao entre os processos:
// MPI_Send(
//     void* data,
//     int count,  // numero de elementos a enviar
//     MPI_Datatype datatype, // tipo dos elementos a serem enviados
//     int destination, // identificador do processo destino
//     int tag,  // etiqueta da mensagem
//     MPI_Comm communicator)
// MPI_Recv(
//     void* data,
//     int count,
//     MPI_Datatype datatype,
//     int source,
//     int tag,
//     MPI_Comm communicator,
//     MPI_Status* status)

int main(int argc, char *argv[])
{
   int i, j, k, id, p, nWorkers, offset, stop, step_size, rows;
   double elapsed_time;
   MPI_Status status; // estrutura que guarda o estado de retorno  
   stop = offset = 0;
   rows = SIZE;

   MPI_Init(&argc, &argv); // inicializa o MPI, recebe o endereço dos parâmetros da função main()
   MPI_Comm_size(MPI_COMM_WORLD, &p); // pega informacao do numero de processos em execucao (quantidade total)
   MPI_Comm_rank(MPI_COMM_WORLD, &id);  // pega o identificador do processo atual (rank)
   nWorkers = p-1;

   printf("id: %d, p:%d\n", id, p);
   fflush(stdout);

   // Escravo (vai utilizar openMP)
   if (id != 0)
   {
      printf("HERE-1\n");fflush(stdout);
      MPI_Recv(&m2, SIZE*SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      printf("HERE\n");fflush(stdout);
      while(1){
         printf("HERE1\n");fflush(stdout);
         MPI_Recv(&stop, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
         printf("HERE1.1\n");fflush(stdout);
         if(stop) break;

         MPI_Recv(&offset, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
         MPI_Recv(&step_size, 1, MPI_INT, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &status);
         MPI_Recv(&m1[offset][0], step_size*SIZE, MPI_DOUBLE, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
         
         
         multiplica_matriz(i, j, k, offset,  step_size);

         MPI_Send(&offset, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
         MPI_Send(&step_size, 1, MPI_INT, i, 4, MPI_COMM_WORLD);
         MPI_Send(&m1[offset][0], step_size*SIZE, MPI_INT ,i, 5, MPI_COMM_WORLD);

         printf("stop: %d, offset: %d, step_size: %d, id: %d", stop, offset, step_size, id);fflush(stdout);

         
      }
      printf("STOP\n"); fflush(stdout);
      MPI_Finalize();
      exit(0);
   } 
   // Mestre
   else
   {
      printf("nWorkers %d", nWorkers);fflush(stdout);
      inicializa_matriz(i, j, k);

      // PREPARA PARA MEDIR TEMPO
      elapsed_time = -MPI_Wtime();

      for(i = 1; i <= nWorkers; i++){
         printf("Sending to %d\n", i);fflush(stdout);
         MPI_Send(&m2, SIZE*SIZE, MPI_INT, i, 1, MPI_COMM_WORLD);
      }

      while(rows > 0) {
         for(i = 1; i <= nWorkers; i++){
            step_size = i == 1 ? 15 : 16;
            step_size = step_size > rows ? rows : step_size;
            MPI_Send(&stop, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
            MPI_Send(&offset, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
            MPI_Send(&step_size, 1, MPI_INT, i, 4, MPI_COMM_WORLD);
            MPI_Send(&m1[offset][0], step_size*SIZE, MPI_INT ,i,5, MPI_COMM_WORLD);
            offset = offset + step_size;
            rows = rows - step_size;
            if(rows <= 0) break;
         }
      }
      stop = 1;
      for(i = 1; i <= nWorkers; i++){
         MPI_Send(&stop, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
      }

      /*MPI_Finalize();
      return 0;*/
      multiplica_matriz(i, j, k);

      // OBTEM O TEMPO
      elapsed_time += MPI_Wtime();

      // VERIFICA SE O RESULTADO DA MULTIPLICACAO ESTA CORRETO
      int correct_mult = valida_multiplicacao(i, j, k);

      // MOSTRA O TEMPO DE EXECUCAO
      printf("Correct result? %s\n", !correct_mult ? "True" : "False");
      printf("%lf\n", elapsed_time);
      MPI_Finalize();
      return correct_mult;
   }
}

int multiplica_matriz(int i, int j, int k, int offset, int step_size)
{
   for (i = offset; i < step_size; i++)
   {
      for (j = 0; j < cres; j++)
      {
         mres[i][j] = 0;
         for (k = 0; k < c1; k++)
         {
            mres[i][j] += m1[i][k] * m2[k][j];
         }
      }
   }
}

int inicializa_matriz(int i, int j, int k)
{
   // INICIALIZA OS ARRAYS A SEREM MULTIPLICADOS
   l1 = c1 = SIZE;
   l2 = c2 = SIZE;
   if (c1 != l2)
   {
      fprintf(stderr, "Impossivel multiplicar matrizes: parametros invalidos.\n");
      return 1;
   }
   lres = l1;
   cres = c2;
   k = 1;
   for (i = 0; i < SIZE; i++)
   {
      for (j = 0; j < SIZE; j++)
      {
         if (k % 2 == 0)
            m1[i][j] = -k;
         else
            m1[i][j] = k;
      }
      k++;
   }
   k = 1;
   for (j = 0; j < SIZE; j++)
   {
      for (i = 0; i < SIZE; i++)
      {
         if (k % 2 == 0)
            m2[i][j] = -k;
         else
            m2[i][j] = k;
      }
      k++;
   }
}

int valida_multiplicacao(int i, int j, int k)
{
   for (i = 0; i < SIZE; i++)
   {
      k = SIZE * (i + 1);
      for (j = 0; j < SIZE; j++)
      {
         int k_col = k * (j + 1);
         if (i % 2 == 0)
         {
            if (j % 2 == 0)
            {
               if (mres[i][j] != k_col)
                  return 1;
            }
            else
            {
               if (mres[i][j] != -k_col)
                  return 1;
            }
         }
         else
         {
            if (j % 2 == 0)
            {
               if (mres[i][j] != -k_col)
                  return 1;
            }
            else
            {
               if (mres[i][j] != k_col)
                  return 1;
            }
         }
      }
   }
   return 0;
}